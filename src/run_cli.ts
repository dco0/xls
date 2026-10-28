
import * as fs from 'node:fs/promises';
import {join} from 'node:path';

import {XLSError, ULP} from './base.js';
import {runScriptFromFile} from './scripts.js';


const HELP_PREFIX = `
XLS (eXtensible Life Search) is a search program for cellular automata.
If you don't know what that means, see https://conwaylife.com/.

There are two ways to use this program.

The first way is to use a script:
    ./xls script <file path>
This executes that XLS script and search for the thing it specifies.

The second is to use a preset mode, this is typically the one you
want to do for simple searches:
    ./xls <rule> <mode> <args>
    ./xls <minrule> <maxrule> <mode> <args>

In all usages, options can be provided by prefixing them with a hyphen.

Options:
`.trim();

type BaseOption<Name extends string> = {name: Name, desc: string, aliases: string[]};
type FlagOption<Name extends string = string> = BaseOption<Name> & {type: 'flag'};
type BooleanOption<Name extends string = string> = BaseOption<Name> & {type: 'boolean'};
type NumberOption<Name extends string = string> = BaseOption<Name> & {type: 'number'};
type StringOption<Name extends string = string> = BaseOption<Name> & {type: 'string'};
type ValidatorResult<T> = {isError: false, value: T} | {isError: true, type: string, reason: string};
type Validator<T> = (value: string) => ValidatorResult<T> | Promise<ValidatorResult<T>>;
type CustomOption<Name extends string = string, Type = unknown> = BaseOption<Name> & {type: 'custom', func: Validator<Type>};
type Option<Name extends string = string> = FlagOption<Name> | BooleanOption<Name> | NumberOption<Name> | StringOption<Name> | CustomOption<Name>;

type ValueOfOption<T extends Option> =
    T extends FlagOption ? true :
    T extends BooleanOption ? boolean :
    T extends NumberOption ? number :
    T extends StringOption ? string :
    T extends CustomOption<string, infer T> ? T :
    never
;

function flagOption<Name extends string>(name: Name, desc: string, aliases: string[] = []): FlagOption<Name> {
    return {name, desc, aliases, type: 'flag'};
}

function booleanOption<Name extends string>(name: Name, desc: string, aliases: string[] = []): BooleanOption<Name> {
    return {name, desc, aliases, type: 'boolean'};
}

function numberOption<Name extends string>(name: Name, desc: string, aliases: string[] = []): NumberOption<Name> {
    return {name, desc, aliases, type: 'number'};
}

function stringOption<Name extends string>(name: Name, desc: string, aliases: string[] = []): StringOption<Name> {
    return {name, desc, aliases, type: 'string'};
}

function customOption<Name extends string, Type>(name: Name, func: Validator<Type>, desc: string, aliases: string[] = []): CustomOption<Name, Type> {
    return {name, desc, aliases, type: 'custom', func};
}

const OPTIONS = [

    flagOption('help', 'Prints this help message', ['h']),

    numberOption('debug', `Sets the debug level`, ['d']),

] as const;

type OptionData = typeof OPTIONS;
type OptionKey = OptionData[number]['name'];
type ParsedOptions = {[K in OptionKey]?: ValueOfOption<Extract<OptionData[number], {name: K}>>};

let optionsWithAliases: {[key: string]: Option} = {};
for (let value of OPTIONS) {
    optionsWithAliases[value.name] = value;
    for (let alias of value.aliases) {
        optionsWithAliases[alias] = value;
    }
}

let options: ParsedOptions = {};
let posArgs: string[] = [];

for (let i = 2; i < process.argv.length; i++) {
    let arg = process.argv[i];
    if (!arg.match(/^-[-a-zA-Z0-9]/)) {
        posArgs.push(arg);
        continue;
    }
    let value: string | undefined;
    if (arg.includes('=')) {
        let index = arg.indexOf('=');
        value = arg.slice(index + 1);
        arg = arg.slice(0, index);
    } else {
        value = undefined;
    }
    let originalArg = arg;
    arg = arg.toLowerCase();
    while (arg.startsWith('-')) {
        arg = arg.slice(1);
    }
    let argData = optionsWithAliases[arg];
    if (argData === undefined) {
        throw new XLSError(`Unrecognized option: '${arg}'`);
    }
    let key = argData.name;
    if (argData.type === 'flag') {
        if (value !== undefined) {
            throw new XLSError(`Cannot provide value for argument ${originalArg}, is a flag argument`);
        }
        // @ts-ignore
        options[key] = true;
    } else if (value === undefined) {
        throw new XLSError(`No value provided for argument ${originalArg} (to provide it, do '${originalArg}=<value>', not '${originalArg} value')`);
    } else if (argData.type === 'boolean') {
        if (value !== 'true' && value !== 'false') {
            throw new XLSError(`Invalid value for argument ${originalArg}, expected 'true' or 'false'`);
        }
        // @ts-ignore
        options[key] = value === 'true';
    } else if (argData.type === 'string') {
        // @ts-ignore
        options[key] = value;
    } else if (argData.type === 'number') {
        let num = Number(value);
        if (Number.isNaN(num)) {
            throw new XLSError(`Invalid numeric value for ${originalArg}: '${value}'`);
        }
        // @ts-ignore
        options[key] = num;
    } else if (argData.type === 'custom') {
        let out = await argData.func(value);
        if (out.isError) {
            throw new XLSError(`Invalid value for ${originalArg}: '${value}' (expected ${out.type}, ${out.reason})`);
        }
        // @ts-ignore
        options[key] = out.value;
    } else {
        throw new Error(`This error should not occur (invalid option: ${argData satisfies never})`);
    }
}

if (options.help) {
    let msg = HELP_PREFIX;
    for (let option of OPTIONS) {
        msg += `\n\n`;
        let line = ' '.repeat(4);
        for (let alias of option.aliases) {
            if (alias.length === 1) {
                line += `-${alias}, `;
            } else {
                line += `--${alias}, `;
            }
        }
        line += `--${option.name}`;
        if (option.type !== 'flag') {
            line += `=<${option.type}>`;
        }
        line += `:`;
        for (let word of option.desc.split(' ')) {
            let oldLine = line;
            line += ' ' + word;
            if (line.length > 80) {
                msg += `\n` + oldLine;
                line = ' '.repeat(8) + word;
            }
        }
        msg += line;
    }
    msg += `\n`;
    console.log(msg);
    process.exit(0);
}


const BASE_PATH = join(import.meta.dirname, '..');

async function searchForULP(ulp: ULP): Promise<void> {
    await fs.writeFile(join(BASE_PATH, 'search.txt'), ulp.toString());
    throw new Error('FILL THIS FUNCTION IN');
}


if (posArgs[0] === 'script') {
    if (posArgs.length !== 2) {
        throw new XLSError(`Invalid number of arguments for script mode (expected 2, got ${posArgs.length})`);
    }
    let ulp = await runScriptFromFile(posArgs[1]);
    await searchForULP(ulp);
    process.exit(0);
}


throw new XLSError(`Invalid invocation of XLS`);
