
// TO FIX
// @ts-ignore
import {LifewebError, IdentityPattern, MAPPattern, MAPB0Pattern, createPattern} from '../lifeweb/src/core/index.js';
import {XLSError, Cell, OFF, ON, UNKNOWN, ClauseFunc, Coord, GridCell, GridLike, Grid, GridSlice, ULP} from './base.js';


interface Position {
    file: string;
    line: number;
    col: number;
}

const BUILTIN_POS = Object.freeze({file: '<builtin>', line: 1, col: 1} satisfies Position) as Position;

function positionToString(pos: Position): string {
    if (pos === BUILTIN_POS) {
        return `<builtin>`;
    } else {
        return `${pos.file}:${pos.line}:${pos.col}`;
    }
}

type ObjWithPos = {pos: Position};

type HasPos = Position | ObjWithPos;

class XLSScriptError extends XLSError {

    name: string = 'XLSScriptError';
    [Symbol.toStringTag]: string = 'XLSScriptError';

}


type NullValue = ObjWithPos & {type: 'null'};
type BooleanValue = ObjWithPos & {type: 'boolean', value: boolean};
type NumberValue = ObjWithPos & {type: 'number', value: number};
type StringValue = ObjWithPos & {type: 'string', chars: number[]};

function toStringChars(value: string): number[] {
    return Array.from(value).map(x => x.codePointAt(0) as number);
}

function getStringValue(value: StringValue): string {
    let out = '';
    for (let char of value.chars) {
        out += String.fromCharCode(char);
    }
    return out;
}

type Range = ObjWithPos & {type: 'range', start: number | undefined, end: number | undefined};

type Vector = ObjWithPos & {type: 'vector', data: number[]};

type VectorRange = ObjWithPos & {type: 'vector_range', data: [number | undefined, number | undefined][]};

// -1 is unknown
type CellSpecifier = ObjWithPos & {type: 'cell_specifier'} & (
    | {kind: 'nop', func: ClauseFunc | undefined}
    | {kind: 'cell', cell: GridCell}
    | {kind: 'periodic', period: number, func: ClauseFunc | undefined}
);

function cellSpecifiersAreEqual(x: CellSpecifier, y: CellSpecifier): boolean {
    if (x.kind !== y.kind) {
        return false;
    }
    if (x.kind === 'nop') {
        return y.kind === 'nop' && x.func === y.func;
    } else if (x.kind === 'cell') {
        return y.kind === 'cell' && x.cell.eq(y.cell);
    } else if (x.kind === 'periodic') {
        return y.kind === 'periodic' && x.period === y.period && x.func === y.func;
    } else {
        throw new Error(`This error should not occur (invalid cell specifier: ${x satisfies never})`);
    }
}

type FullCellSpecifierPart = {isRelative: boolean, gens: NumberValue | Range, value: CellSpecifier};
type FullCellSpecifier = ObjWithPos & {type: 'full_cell_specifier', parts: FullCellSpecifierPart[]};

type GridValue = ObjWithPos & {type: 'grid', grid: Grid};
type GridSliceValue = ObjWithPos & {type: 'grid_slice', grid: GridSlice};

type BuiltinFunctionFunc = (this: XLSScriptRunner, pos: Position, ...args: (Value | undefined)[]) => Value;
type FunctionValue = ObjWithPos & {type: 'function', name: string} & (
    | {builtin: false, closure: Scope, params: IdentifierToken[], body: BlockStatement}
    | {builtin: true, params: string[], func: BuiltinFunctionFunc}
) & {boundArgs?: Value[]};


type BaseLvalue = ObjWithPos & {type: 'lvalue'};
type IdentifierLvalue = BaseLvalue & {kind: 'identifier', scope: Scope, name: string};
type StateLvalue = BaseLvalue & {kind: 'state', scope: Scope, value: number};
type GridCoordLvalue = BaseLvalue & {kind: 'grid_coord', grid: GridLike, coord: Coord};
const GRID_MAGIC_KEYS = new Set([
    'height', 'width', 'gens',
] as const);
type GridMagicKey = typeof GRID_MAGIC_KEYS extends Set<infer T> ? T : never;
type GridMagicKeyLvalue = BaseLvalue & {kind: 'grid_magic_key', grid: GridLike, key: GridMagicKey};
type Lvalue = IdentifierLvalue | StateLvalue | GridCoordLvalue | GridMagicKeyLvalue;

function copyLvalue<T extends Lvalue>(value: T): T {
    let pos = value.pos;
    let out: Lvalue;
    if (value.kind === 'identifier') {
        out = {pos, type: 'lvalue', kind: 'identifier', scope: value.scope, name: value.name};
    } else if (value.kind === 'state') {
        out = {pos, type: 'lvalue', kind: 'state', scope: value.scope, value: value.value};
    } else if (value.kind === 'grid_coord') {
        out = {pos, type: 'lvalue', kind: 'grid_coord', grid: value.grid, coord: value.coord.copy()};
    } else if (value.kind === 'grid_magic_key') {
        out = {pos, type: 'lvalue', kind: 'grid_magic_key', grid: value.grid, key: value.key};
    } else {
        throw new Error(`This error should not occur (invalid value: ${value satisfies never})`);
    }
    return out as T;
}

function resolveGridCoordLvalue(data: GridCoordLvalue): {grid: GridLike, coord: Coord} {
    if (data.grid instanceof GridSlice) {
        let coord = data.coord.add(data.grid.start);
        return {grid: data.grid, coord};
    } else {
        return data;
    }
}

function lvaluesAreEqual(x: Lvalue, y: Lvalue): boolean {
    if (x.kind === 'identifier') {
        return y.kind === 'identifier' && x.scope === y.scope && x.name === y.name;
    } else if (x.kind === 'state') {
        return y.kind === 'state' && x.scope === y.scope && x.value === y.value;
    } else if (x.kind === 'grid_coord') {
        if (y.kind !== 'grid_coord') {
            return false;
        }
        let x2 = resolveGridCoordLvalue(x);
        let y2 = resolveGridCoordLvalue(y);
        return x2.grid === y2.grid && x2.coord.eq(y2.coord);
    } else if (x.kind === 'grid_magic_key') {
        return y.kind === 'grid_magic_key' && x.grid === y.grid && x.key === y.key;
    } else {
        throw new Error(`This error should not occur (invalid lvalue: ${x satisfies never})`);
    }
}

type Reference = ObjWithPos & {type: 'reference', value: Lvalue};


type Value = NullValue | BooleanValue | NumberValue | StringValue | Range | Vector | VectorRange | CellSpecifier | FullCellSpecifier | GridValue | GridSliceValue | FunctionValue | Reference;

type ValueType = Value['type'];
type ValueOfType<T extends ValueType> = Extract<Value, {type: T}>;

function copyValue<T extends Value>(value: T): T {
    let pos = value.pos;
    let out: Value;
    if (value.type === 'null' || value.type === 'boolean' || value.type === 'number' || value.type === 'string' || value.type === 'range' || value.type === 'vector' || value.type === 'vector_range') {
        out = structuredClone(value);
    } else if (value.type === 'cell_specifier') {
        if (value.kind === 'nop') {
            out = {pos, type: 'cell_specifier', kind: 'nop', func: value.func};
        } else if (value.kind === 'cell') {
            out = {pos, type: 'cell_specifier', kind: 'cell', cell: value.cell.copy()};
        } else if (value.kind === 'periodic') {
            out = {pos, type: 'cell_specifier', kind: 'periodic', period: value.period, func: value.func};
        } else {
            throw new Error(`This error should not occur (invalid cell specifier: ${value satisfies never})`);
        }
    } else if (value.type === 'full_cell_specifier') {
        let parts: FullCellSpecifierPart[] = [];
        for (let part of value.parts) {
            parts.push({
                gens: structuredClone(part.gens),
                isRelative: part.isRelative,
                value: copyValue(part.value),
            });
        }
        out = {pos, type: 'full_cell_specifier', parts};
    } else if (value.type === 'grid') {
        out = {pos, type: 'grid', grid: value.grid};
    } else if (value.type === 'grid_slice') {
        out = {pos, type: 'grid_slice', grid: value.grid};
    } else if (value.type === 'function') {
        if (value.builtin) {
            out = {pos, type: 'function', name: value.name, builtin: true, params: structuredClone(value.params), func: value.func};
        } else {
            out = {pos, type: 'function', name: value.name, builtin: false, closure: value.closure, params: structuredClone(value.params), body: value.body};
        }
        if (value.boundArgs) {
            out.boundArgs = value.boundArgs.map(copyValue);
        }
    } else if (value.type === 'reference') {
        out = {pos, type: 'reference', value: copyLvalue(value.value)};
    } else {
        throw new Error(`This error should not occur (invalid value: ${value satisfies never})`);
    }
    return out as T;
}

function valueIsTruthy(value: Value): boolean {
    if (value.type === 'null') {
        return false;
    } else if (value.type === 'boolean') {
        return value.value;
    } else if (value.type === 'number') {
        return value.value !== 0;
    } else if (value.type === 'string') {
        return value.chars.length > 0;
    } else if (value.type === 'range') {
        return value.start === undefined || value.end === undefined || value.start > value.end;
    } else if (value.type === 'vector') {
        return value.data.length > 0;
    } else if (value.type === 'vector_range') {
        for (let i = 0; i < value.data.length; i++) {
            let item = value.data[i];
            if (item[0] === undefined || item[1] === undefined) {
                continue;
            }
            if (item[1] > item[0]) {
                return false;
            }
        }
        return true;
    } else {
        value satisfies (CellSpecifier | FullCellSpecifier | GridValue | GridSliceValue | FunctionValue | Reference);
        return true;
    }
}

function valuesAreEqual(x: Value, y: Value): boolean {
    if (x.type === 'null') {
        return y.type === 'null';
    } else if (x.type === 'boolean' || x.type === 'number') {
        return x.type === y.type && x.value === y.value;
    } else if (x.type === 'string') {
        if (y.type !== 'string' || x.chars.length !== y.chars.length) {
            return false;
        }
        for (let i = 0; i < x.chars.length; i++) {
            if (x.chars[i] !== y.chars[i]) {
                return false;
            }
        }
        return true;
    } else if (x.type === 'range') {
        return y.type === 'range' && x.start === y.start && x.end === y.end;
    } else if (x.type === 'vector') {
        if (y.type !== 'vector' || x.data.length !== y.data.length) {
            return false;
        }
        for (let i = 0; i < x.data.length; i++) {
            if (x.data[i] !== y.data[i]) {
                return false;
            }
        }
        return true;
    } else if (x.type === 'vector_range') {
        if (y.type !== 'vector_range' || x.data.length !== y.data.length) {
            return false;
        }
        for (let i = 0; i < x.data.length; i++) {
            let xItem = x.data[i];
            let yItem = y.data[i];
            if (!(xItem[0] === yItem[0] && xItem[1] === yItem[1])) {
                return false;
            }
        }
        return true;
    } else if (x.type === 'cell_specifier') {
        return y.type === 'cell_specifier' && cellSpecifiersAreEqual(x, y);
    } else if (x.type === 'full_cell_specifier' || x.type === 'grid' || x.type === 'grid_slice' || x.type === 'function') {
        return x === y;
    } else if (x.type === 'reference') {
        return y.type === 'reference' && lvaluesAreEqual(x.value, y.value);
    } else {
        throw new Error(`This error should not occur (invalid value: ${x satisfies never})`);
    }
}

type ExpressionValue = Value | Lvalue;

function isLvalue(value: ExpressionValue): value is Lvalue {
    return value !== null && typeof value === 'object' && value.type === 'lvalue';
}


const WHITESPACE = new Set([' ', '\n', '\t', '\v', '\f']);

const KEYWORDS = new Set([
    // literals
    'null', 'true', 'false', 'nop', 'off', 'on', 'unknown', 'var', 'all',
    // operators
    'state', 'noclause', 'period', 'typeof',
    // statements
    'let', 'mut', 'if', 'else', 'for', 'while',
] as const);
type Keyword = typeof KEYWORDS extends Set<infer T> ? T : never;

const SYMBOLS = new Set([
    // misc
    ',', ':', ';', '(', ')', '{', '}', '[', ']',
    // postfix operators
    '.',
    // unary operators
    '+', '-', '~', '!', '%', '&', '*',
    // arithmetic binary operators
    '+', '-', '*', '/', '%', '<<', '>>', '&', '^', '|', '..', '..=',
    // boolean binary operators
    '<', '<=', '>', '>=', '==', '!=', '&&', '||', '^^', '??', '?', ':',
    // assignment operators
    '=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=', '??=',
] as const);
type Symbol_ = typeof SYMBOLS extends Set<infer T> ? T : never;

const UNARY_OPERATORS = new Set([
    '+', '-', '~', '!', '&', '*',
    'state', 'noclause', 'period', 'typeof',
] as const);
type UnaryOperator = typeof UNARY_OPERATORS extends Set<infer T> ? T : never;

const BINARY_OPERATORS = new Set([
    '+', '-', '*', '/', '%', '<<', '>>', '&', '^', '|',
    '<', '<=', '>', '>=', '==', '!=', '&&', '||', '^^', '??',
    '=',
] as const);
type BinaryOperator = typeof BINARY_OPERATORS extends Set<infer T> ? T : never;


const EOF = Symbol('EOF');
type EOF = typeof EOF;

type NewlineToken = ObjWithPos & {type: 'newline'};
type KeywordToken<T extends Keyword = Keyword> = ObjWithPos & {type: 'keyword', value: T};
type IdentifierToken = ObjWithPos & {type: 'identifier', name: string};
type NumberLiteralToken = ObjWithPos & {type: 'number-literal', value: number, raw: string};
type StringLiteralToken = ObjWithPos & {type: 'string-literal', value: number[], raw: string};
type SymbolToken<T extends Symbol_ = Symbol_> = ObjWithPos & {type: 'symbol', value: T};
type RLEToken = ObjWithPos & {type: 'rle', value: string};
type Token = NewlineToken | KeywordToken | IdentifierToken | NumberLiteralToken | StringLiteralToken | SymbolToken | RLEToken;

const STRING_TOKEN_NAMES: {[K in Token['type']]: string} = {
    'newline': 'newline',
    'keyword': 'keyword',
    'identifier': 'identifier',
    'number-literal': 'number literal',
    'string-literal': 'string literal',
    'symbol': 'symbol',
    'rle': 'RLE',
};

function getTokenRaw(token: Token): string {
    if (token.type === 'newline') {
        return '\\n';
    } else if (token.type === 'identifier') {
        return token.name;
    } else if (token.type === 'number-literal') {
        return token.raw;
    } else if (token.type === 'string-literal') {
        return token.raw;
    } else {
        return JSON.stringify(token.value satisfies string).slice(1, -1).replaceAll('\\"', '"');
    }
}

function tokenToString(token: Token | EOF): string {
    if (token === EOF) {
        return `end of file`;
    } else {
        return `${STRING_TOKEN_NAMES[token.type]} '${getTokenRaw(token)}'`;
    }
}


type Matcher = 
    | EOF
    | 'any'
    | Token['type']
    | Keyword
    | Symbol_
    | `identifier ${string}`
    | Matcher[]
    | Set<Matcher>
;

const LINE_END: ['newline', ';', EOF] = ['newline', ';', EOF];

type MatcherReturnType<T extends Matcher> =
    T extends EOF ? EOF :
    T extends 'any' ? Token :
    T extends Token['type'] ? Extract<Token, {type: T}> :
    T extends Keyword ? KeywordToken<T> :
    T extends Symbol_ ? SymbolToken<T> :
    T extends `identifier ${string}` ? IdentifierToken :
    // T extends (infer U)[] ? MatcherReturnType<U> :
    T extends Matcher[] ? Token :
    T extends Set<Matcher> ? Token :
    never
;

function matcherToString(matcher: Matcher): string {
    if (matcher === EOF) {
        return `end of file`;
    } else if (typeof matcher === 'string') {
        if (KEYWORDS.has(matcher as Keyword)) {
            return `keyword '${matcher}'`;
        } else if (SYMBOLS.has(matcher as Symbol_)) {
            return `symbol '${matcher}'`;
        } else if (matcher.startsWith('identifier ')) {
            return `identifier '${matcher.slice('identifier '.length)}'`;
        } else {
            return STRING_TOKEN_NAMES[matcher as Token['type']];
        }
    } else {
        let out = Array.from(matcher).map(matcherToString);
        if (out.length === 0) {
            return `nothing`;
        } else if (out.length === 1) {
            return out[0];
        } else if (out.length === 2) {
            return `${out[0]} or ${out[1]}`;
        } else {
            return `${out.slice(0, -1).join(', ')}, or ${out[out.length - 1]}`;
        }
    }
}


type IdentifierExpression = ObjWithPos & {type: 'identifier-expression', name: string};
type SimpleLiteralExpression = ObjWithPos & {type: 'simple-literal-expression', value: Value};
type CellLiteralExpression = ObjWithPos & {type: 'cell-literal-expression', value: number};
type VectorLiteralExpression = ObjWithPos & {type: 'vector-literal-expression', data: Expression[]};
type FunctionCallExpression = ObjWithPos & {type: 'function-call-expression', func: Expression, args: Expression[]};
type MemberExpression = ObjWithPos & {type: 'member-expression', value: Expression, member: IdentifierToken | Expression};
type UnaryExpression = ObjWithPos & {type: 'unary-expression', op: UnaryOperator, value: Expression};
type RangeExpression = ObjWithPos & {type: 'range-expression', isInclusive: boolean, start: Expression | undefined, end: Expression | undefined};
type BinaryExpression = ObjWithPos & {type: 'binary-expression', op: BinaryOperator, left: Expression, right: Expression};
type FullCellSpecifierExpressionPart = ObjWithPos & {type: 'full-state-specifier-expression-part', isRelative: boolean, gens: Expression, value: Expression};
type FullCellSpecifierExpression = ObjWithPos & {type: 'full-state-specifier-expression', parts: FullCellSpecifierExpressionPart[]};
type ConditionalExpression = ObjWithPos & {type: 'conditional-expression', condition: Expression, ifTrue: Expression, ifFalse: Expression};
type Expression = IdentifierExpression | SimpleLiteralExpression | CellLiteralExpression | VectorLiteralExpression | FunctionCallExpression | MemberExpression | UnaryExpression | RangeExpression | BinaryExpression | FullCellSpecifierExpression | ConditionalExpression;
type AnyExpressionNode = Expression | FullCellSpecifierExpressionPart;

type ExpressionStatement = ObjWithPos & {type: 'expression-statement', expr: Expression};
type BlockStatement = ObjWithPos & {type: 'block-statement', data: Statement[]};
type RLEStatement = ObjWithPos & {type: 'rle-statement', grid: Expression, gens?: Expression, rle: RLEToken};
type VariableDeclarationStatement = ObjWithPos & {type: 'variable-declaration-statement', mut: boolean, name: string, value?: Expression};
type IfStatement = ObjWithPos & {type: 'if-statement', condition: Expression, ifTrue: BlockStatement, ifFalse?: BlockStatement};
type ForStatement = ObjWithPos & {type: 'for-statement', mut: boolean, name: IdentifierToken, iterateOver: Expression, value: BlockStatement};
type WhileStatement = ObjWithPos & {type: 'while-statement', condition: Expression, value: BlockStatement};
type Statement = ExpressionStatement | BlockStatement | RLEStatement | VariableDeclarationStatement | IfStatement | ForStatement | WhileStatement;
type AnyStatementNode = Statement;

type Node = AnyExpressionNode | AnyStatementNode;
type NodeType = Node['type'];


class XLSScriptParser {

    readonly ulp: ULP;

    currentFilePath: string;
    code: string;
    tokens: Token[];
    pos: number;

    constructor(filePath: string, code: string, ulp: ULP) {
        this.currentFilePath = filePath;
        this.code = code;
        this.ulp = ulp;
        this.tokens = this.tokenize(code);
        // console.log(this.tokens.map(x => {
        //     let y = structuredClone(x);
        //     // @ts-ignore
        //     delete y.pos;
        //     return y;
        // }));
        this.pos = 0;
    }

    getCurrentPosition(): Position {
        if (this.tokens[this.pos] === undefined) {
            return this.tokens[this.tokens.length - 1].pos;
        } else {
            return this.tokens[this.pos].pos;
        }
    }

    error(pos: HasPos | undefined, type: string, msg: string): never {
        if (pos === undefined) {
            pos = this.getCurrentPosition();
        } else if ('pos' in pos) {
            pos = pos.pos;
        }
        let text = `${type}: ${msg}\n    at ${positionToString(pos)}`;
        throw new XLSScriptError(text);
    }

    readonly ESCAPE_SEQUENCE_REGEX = /^((\\)(['"?\\abfnrtv]|[0-7]{1,3}|o\{[0-7]{1,3}\}|x[0-9a-fA-F]{2}|u\{[0-9a-fA-F]+\}))/u;

    parseEscapeSequence(value: string): string {
        value = value.slice(1);
        if (value === `'` || value === '"' || value === '?' || value === '\\') {
            return value;
        } else if (value === 'a') {
            return '\x07';
        } else if (value === 'b') {
            return '\b';
        } else if (value === 'f') {
            return '\f';
        } else if (value === 'n') {
            return '\n';
        } else if (value === 'r') {
            return '\r';
        } else if (value === 't') {
            return '\t';
        } else if (value === 'v') {
            return '\v';
        } else if (value === '0') {
            return '\0';
        } else if (value.startsWith('x')) {
            return String.fromCodePoint(parseInt(value.slice(1)));
        } else if (value.startsWith('u')) {
            value = value.slice(2, -1);
            return String.fromCodePoint(parseInt(value, 8));
        } else {
            throw new Error(`This error should not occur, please report it (invalid escape sequence)`);
        }
    }

    parseStringLiteral(value: string): number[] | false {
        if (!((value.startsWith('"') && value.endsWith('"')) || (value.startsWith(`'`) && value.endsWith(`'`)))) {
            return false;
        }
        let out: number[] = [];
        // call Array.from here to use real Unicode characters
        let chars = Array.from(value.slice(1, -1));
        for (let i = 0; i < chars.length; i++) {
            let char = chars[i];
            if (char === '"') {
                return false;
            } else if (char === '\\') {
                let match = chars.slice(i).join('').match(this.ESCAPE_SEQUENCE_REGEX);
                if (!match) {
                    return false;
                }
                i += Array.from(match[0]).length - 1;
                out.push(this.parseEscapeSequence(match[0]).codePointAt(0) as number);
            } else {
                out.push(char.codePointAt(0) as number);
            }
        }
        return out;
    }

    readonly IDENTIFIER_REGEX = /^\p{XID_Start}\p{XID_Continue}*$/u;
    readonly NUMBER_REGEX = /^-?(0|[1-9]\d*(.[0-9]+)?(e[+-]?[1-9]\d+(.[0-9]+)?)?|0b[01]+|0o[0-7]+|0x[0-9a-f])$/iu;

    tryToConvertToSingleToken(pos: Position, value: string): Token | false | undefined {
        pos = Object.freeze(structuredClone(pos));
        let string = this.parseStringLiteral(value);
        if (string !== false) {
            return {pos, type: 'string-literal', value: string, raw: value};
        }
        if (WHITESPACE.has(value)) {
            // undefined means whitespace
            return undefined;
        } else if (value.match(this.IDENTIFIER_REGEX)) {
            if (KEYWORDS.has(value as Keyword)) {
                return {pos, type: 'keyword', value: value as Keyword};
            } else {
                return {pos, type: 'identifier', name: value};
            }
        } else if (value.match(this.NUMBER_REGEX)) {
            return {pos, type: 'number-literal', value: Number(value), raw: value};
        } else if (SYMBOLS.has(value as Symbol_)) {
            return {pos, type: 'symbol', value: value as Symbol_};
        } else {
            return false;
        }
    }

    readonly RLE_HEADER_REGEX = /^x\s*=\s*(\d+)\s*,\s*y\s*=\s*(\d+)\s*(,\s*rule\s*=\s*.*\s*)?$/;

    tokenize(code: string): Token[] {
        let file = this.currentFilePath;
        // call Array.from here to use real Unicode characters
        let lines = code.split('\n').map(line => Array.from(line));
        let out: Token[] = [];
        let multiLineComment: false | Position = false;
        for (let lineNumber = 1; lineNumber < lines.length + 1; lineNumber++) {
            let line: string[] = [];
            let startLineNumber = lineNumber;
            while (true) {
                for (let char of lines[lineNumber - 1]) {
                    line.push(char);
                }
                if (line[line.length - 1] === '\\') {
                    lineNumber++;
                } else {
                    break;
                }
            }
            lineNumber = startLineNumber;
            let currentLine: Token[] = [];
            let singleLineComment = false;
            let col = 1;
            while (WHITESPACE.has(line[0])) {
                line = line.slice(1);
                col++;
            }
            while (WHITESPACE.has(line[line.length - 1])) {
                line = line.slice(0, -1);
            }
            if (line.join().match(this.RLE_HEADER_REGEX)) {
                let pos: Position = {file, line: lineNumber, col};
                let rle = line.join();
                while (true) {
                    lineNumber++;
                    let line = lines[lineNumber - 1];
                    let index = line.indexOf('!');
                    if (index === -1) {
                        rle += '\n' + line.join();
                        continue;
                    } else {
                        rle += '\n' + line.slice(0, index + 1).join();
                        line = line.slice(index + 1);
                        col = index + 2;
                        break;
                    }
                }
                out.push({pos: Object.freeze(pos), type: 'rle', value: rle});
            }
            while (line.length > 0) {
                if (col > lines[lineNumber - 1].length) {
                    col -= lines[lineNumber - 1].length;
                    lineNumber++;
                }
                let pos = {file, line: lineNumber, col};
                let char = line[0];
                // do comments
                if (singleLineComment) {
                    line.shift();
                    col += 1;
                    continue;
                }
                if (multiLineComment) {
                    if (char === '*' && line[1] === '/') {
                        multiLineComment = false;
                        line.shift();
                        line.shift();
                        col += 2;
                        continue;
                    } else {
                        line.shift();
                        col += 1;
                        continue;
                    }
                }
                if (char === '/' && line[1] === '/') {
                    singleLineComment = true;
                    line.shift();
                    line.shift();
                    col += 2;
                    continue;
                } else if (char === '/' && line[1] === '*') {
                    multiLineComment = structuredClone(pos);
                    line.shift();
                    line.shift();
                    col += 2;
                    continue;
                }
                // get the largest possible token
                let found = false;
                for (let i = line.length; i > 0; i--) {
                    let value = line.slice(0, i).join('');
                    let token = this.tryToConvertToSingleToken(pos, value);
                    if (token !== false) {
                        // undefined means whitespace
                        if (token !== undefined) {
                            currentLine.push(token);
                        }
                        found = true;
                        for (let j = 0; j < i; j++) {
                            line.shift();
                        }
                        col += i;
                        break;
                    }
                }
                if (!found) {
                    this.error(pos, 'SyntaxError', `Cannot parse input into token`);
                }
            }
            for (let token of currentLine) {
                out.push(token);
            }
            out.push({pos: Object.freeze({file, line: lineNumber, col}), type: 'newline'});
        }
        if (multiLineComment) {
            this.error(multiLineComment, 'SyntaxError', `Unterminated multi-line comment`);
        }
        return out;
    }

    peek<T extends Exclude<Matcher, EOF> = Exclude<Matcher, EOF>>(): MatcherReturnType<T> {
        let out = this.tokens[this.pos];
        if (out === undefined) {
            this.error(undefined, 'SyntaxError', `Unexpected end of input`);
        } else {
            return out as MatcherReturnType<T>;
        }
    }

    peekOrEOF<T extends Matcher = Matcher>(): MatcherReturnType<T> | EOF {
        return (this.tokens[this.pos] ?? EOF) as MatcherReturnType<T> | EOF;
    }

    advance<T extends Exclude<Matcher, EOF> = Exclude<Matcher, EOF>>(): MatcherReturnType<T> {
        let out = this.tokens[this.pos];
        if (out === undefined) {
            this.error(undefined, 'SyntaxError', `Unexpected end of input`);
        } else {
            this.pos++;
            return out as MatcherReturnType<T>;
        }
    }

    advanceOrEOF<T extends Matcher = Matcher>(): MatcherReturnType<T> | EOF {
        let out = this.tokens[this.pos];
        if (out === undefined) {
            return EOF;
        } else {
            this.pos++;
            return out as MatcherReturnType<T>;
        }
    }

    _match(token: Token | EOF, matcher: Matcher): boolean {
        if (matcher === EOF) {
            return token === EOF;
        } else if (token === EOF) {
            return false;
        } else if (typeof matcher === 'string') {
            if (KEYWORDS.has(matcher as Keyword)) {
                return token.type === 'keyword' && token.value === matcher;
            } else if (SYMBOLS.has(matcher as Symbol_)) {
                return token.type === 'symbol' && token.value === matcher;
            } else if (matcher.startsWith('identifier ')) {
                return token.type === 'identifier' && token.name === matcher.slice('identifier '.length);
            } else {
                return token.type === matcher;
            }
        } else {
            for (let value of matcher) {
                if (this._match(token, value)) {
                    return true;
                }
            }
            return false;
        }
    }

    match(...data: Matcher[]): boolean {
        for (let i = 0; i < data.length; i++) {
            let token = (this.tokens[this.pos + i] ?? EOF) as Token | EOF;
            if (!this._match(token, data[i])) {
                return false;
            }
        }
        return true;
    }

    expect(...data: Matcher[]): void {
        for (let i = 0; i < data.length; i++) {
            let token = (this.tokens[this.pos + i] ?? EOF) as Token | EOF;
            let matcher = data[i];
            if (!this._match(token, matcher)) {
                this.error(undefined, 'SyntaxError', `Expected ${matcherToString(matcher)}, got ${tokenToString(token)}`);
            }
        }
    }

    eat<T extends Exclude<Matcher, EOF>>(matcher: T): MatcherReturnType<T> {
        this.expect(matcher);
        return this.advance<T>();
    }

    try<T, U extends any[]>(func: (this: this, ...args: U) => T, ...args: U): T | undefined {
        try {
            return func.apply(this, args);
        } catch (error) {
            if (error instanceof XLSScriptError) {
                return undefined;
            } else {
                throw error;
            }
        }
    }

    tryVoid<T extends any[]>(func: (this: this, ...args: T) => void, ...args: T): boolean {
        try {
            func.apply(this, args);
            return true;
        } catch (error) {
            if (error instanceof XLSScriptError) {
                return false;
            } else {
                throw error;
            }
        }
    }

    expressionOrVectorLiteral(vectorPos: Position): Expression {
        let value = this.expression();
        if (!this.match(',')) {
            return value;
        }
        this.advance();
        let data: Expression[] = [value];
        while (true) {
            data.push(this.expression());
            if (this.match(',')) {
                this.advance();
                continue;
            } else {
                break;
            }
        }
        return {pos: vectorPos, type: 'vector-literal-expression', data};
    }

    primaryExpression(): Expression {
        if (this.match('identifier')) {
            // identifier lvalue
            let token = this.advance<'identifier'>();
            return {pos: token.pos, type: 'identifier-expression', name: token.name};
        } else if (this.match('null')) {
            // null literal
            let token = this.advance();
            let value: NullValue = {pos: token.pos, type: 'null'};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('false')) {
            // false literal
            let token = this.advance();
            let value: BooleanValue = {pos: token.pos, type: 'boolean', value: false};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('true')) {
            // true literal
            let token = this.advance();
            let value: BooleanValue = {pos: token.pos, type: 'boolean', value: true};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('number-literal')) {
            // number literal
            let token = this.advance<'number-literal'>();
            let value: NumberValue = {pos: token.pos, type: 'number', value: token.value};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('string-literal')) {
            // string literal
            let token = this.advance<'string-literal'>();
            let value: StringValue = {pos: token.pos, type: 'string', chars: token.value};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('nop')) {
            // 'nop' cell specifier literal
            let token = this.advance();
            let value: CellSpecifier = {pos: token.pos, type: 'cell_specifier', kind: 'nop', func: undefined};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match(['off', 'on', 'unknown', 'var'])) {
            // basic cell specifier literal
            let token = this.advance() as KeywordToken<'off' | 'on' | 'unknown' | 'var'>;
            let value: number;
            if (token.value === 'off') {
                value = OFF;
            } else if (token.value === 'on') {
                value = ON;
            } else if (token.value === 'unknown') {
                value = UNKNOWN;
            } else {
                value = this.ulp.getNewVar();
            }
            return {pos: token.pos, type: 'cell-literal-expression', value};
        } else if (this.match('all')) {
            // 'all' range literal
            let token = this.advance();
            let value: Range = {pos: token.pos, type: 'range', start: undefined, end: undefined};
            return {pos: token.pos, type: 'simple-literal-expression', value};
        } else if (this.match('(')) {
            let token = this.advance();
            let out = this.expressionOrVectorLiteral(token.pos);
            this.eat(')');
            return out;
        } else {
            this.error(undefined, 'SyntaxError', `Expected expression`);
        }
    }

    postfixExpression(): Expression {
        let value = this.primaryExpression();
        while (this.match(['(', '.', '['])) {
            let token = this.advance<'(' | '.' | '['>();
            if (token.value === '(') {
                // function call expression
                let args: Expression[] = [];
                while (true) {
                    args.push(this.expression());
                    if (this.match(',')) {
                        this.advance();
                        continue;
                    } else {
                        break;
                    }
                }
                this.eat(')');
                value = {pos: token.pos, type: 'function-call-expression', func: value, args};
            } else {
                // member expression
                let member: IdentifierToken | Expression;
                if (token.value === '.') {
                    if (this.match('identifier')) {
                        member = this.advance<'identifier'>();
                    } else if (this.match('keyword')) {
                        let token = this.advance<'keyword'>();
                        member = {pos: token.pos, type: 'identifier', name: token.value};
                    } else {
                        this.error(undefined, 'SyntaxError', `Expected identifier or keyword`);
                    }
                } else {
                    member = this.expressionOrVectorLiteral(token.pos);
                    this.eat(']');
                }
                value = {pos: token.pos, type: 'member-expression', value, member};
            }
        }
        return value;
    }

    unaryExpression(): Expression {
        if (!this.match(UNARY_OPERATORS)) {
            return this.postfixExpression();
        }
        let token = this.eat(UNARY_OPERATORS) as SymbolToken | KeywordToken;
        let op = token.value as UnaryOperator;
        let value = this.postfixExpression();
        return {pos: token.pos, type: 'unary-expression', op, value};
    }

    rangeExpression(): Expression {
        let op: SymbolToken<'..' | '..='>;
        let start: Expression | undefined;
        let end: Expression | undefined;
        if (this.match(['..', '..='])) {
            op = this.advance<'..' | '..='>();
            start = undefined;
            end = this.unaryExpression();
        } else {
            start = this.unaryExpression();
            if (!this.match(['..', '..='])) {
                return start;
            }
            op = this.advance<'..' | '..='>();
            end = this.try(this.unaryExpression);
        }
        let isInclusive = op.value === '..=';
        return {pos: op.pos, type: 'range-expression', isInclusive, start, end};
    }

    multiplicativeExpression(): Expression {
        let left = this.rangeExpression();
        while (this.match(['*', '/', '%'])) {
            let op = this.advance<'*' | '/' | '%'>();
            let right = this.rangeExpression();
           left = {pos: op.pos, type: 'binary-expression', op: op.value, left, right};
        }
        return left;
    }

    additiveExpression(): Expression {
        let left = this.multiplicativeExpression();
        while (this.match(['+', '-'])) {
            let op = this.advance<'+' | '-'>();
            let right = this.multiplicativeExpression();
            left = {pos: op.pos, type: 'binary-expression', op: op.value, left, right};
        }
        return left;
    }

    shiftExpression(): Expression {
        let left = this.additiveExpression();
        while (this.match(['<<', '>>'])) {
            let op = this.advance<'<<' | '>>'>();
            let right = this.additiveExpression();
            left = {pos: op.pos, type: 'binary-expression', op: op.value, left, right};
        }
        return left;
    }

    relationalExpression(): Expression {
        let left = this.shiftExpression();
        while (this.match(['<', '<=', '>', '>='])) {
            let op = this.advance<'<' | '<=' | '>' | '>='>();
            let right = this.shiftExpression();
            left = {pos: op.pos, type: 'binary-expression', op: op.value, left, right};
        }
        return left;
    }

    equalityExpression(): Expression {
        let left = this.relationalExpression();
        while (this.match(['==', '!='])) {
            let op = this.advance<'==' | '!='>();
            let right = this.relationalExpression();
            left = {pos: op.pos, type: 'binary-expression', op: op.value, left, right};
        }
        return left;
    }

    bitwiseANDExpression(): Expression {
        let left = this.equalityExpression();
        while (this.match('&')) {
            let op = this.advance();
            let right = this.equalityExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '&', left, right};
        }
        return left;
    }

    bitwiseXORExpression(): Expression {
        let left = this.bitwiseANDExpression();
        while (this.match('^')) {
            let op = this.advance();
            let right = this.bitwiseANDExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '^', left, right};
        }
        return left;
    }

    bitwiseORExpression(): Expression {
        let left = this.bitwiseXORExpression();
        while (this.match('|')) {
            let op = this.advance();
            let right = this.bitwiseXORExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '|', left, right};
        }
        return left;
    }

    logicalANDExpression(): Expression {
        let left = this.bitwiseORExpression();
        while (this.match('&&')) {
            let op = this.advance();
            let right = this.bitwiseORExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '&&', left, right};
        }
        return left;
    }

    logicalXORExpression(): Expression {
        let left = this.logicalANDExpression();
        while (this.match('^^')) {
            let op = this.advance();
            let right = this.logicalANDExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '^^', left, right};
        }
        return left;
    }

    logicalORExpression(): Expression {
        let left = this.logicalXORExpression();
        while (this.match('||')) {
            let op = this.advance();
            let right = this.logicalXORExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '||', left, right};
        }
        return left;
    }

    defaultToExpression(): Expression {
        let left = this.logicalORExpression();
        while (this.match('??')) {
            let op = this.advance();
            let right = this.logicalORExpression();
            left = {pos: op.pos, type: 'binary-expression', op: '??', left, right};
        }
        return left;
    }

    fullCellSpecifierExpressionPart(): FullCellSpecifierExpressionPart {
        let isRelative = false;
        if (this.match('%')) {
            this.advance();
            isRelative = true;
        }
        let gens = this.rangeExpression();
        let colon = this.eat(':');
        let value = this.defaultToExpression();
        return {pos: colon.pos, type: 'full-state-specifier-expression-part', isRelative, gens, value};
    }

    fullCellSpecifierExpression(): Expression {
        let isSpecifier = false;
        let startPos = this.pos;
        try {
            this.rangeExpression();
            if (this.match(':')) {
                isSpecifier = true;
            }
        } catch (error) {
            if (!(error instanceof XLSScriptError)) {
                throw error;
            }
        }
        this.pos = startPos;
        if (!isSpecifier) {
            return this.logicalORExpression();
        }
        let pos = this.getCurrentPosition();
        let parts: FullCellSpecifierExpressionPart[] = [];
        while (true) {
            parts.push(this.fullCellSpecifierExpressionPart());
            if (this.match(',')) {
                this.advance();
                continue;
            } else {
                break;
            }            
        }
        return {pos, type: 'full-state-specifier-expression', parts};
    }

    conditionalExpression(): Expression {
       let condition = this.logicalORExpression();
        if (!this.match('?')) {
            return condition;
        }
        let questionMark = this.advance();
        let ifTrue = this.expression();
        this.eat(':');
        let ifFalse = this.conditionalExpression();
        return {pos: questionMark.pos, type: 'conditional-expression', condition, ifTrue, ifFalse};
    }

    expression(): Expression {
        let left = this.conditionalExpression();
        if (!this.match(['=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=', '??='])) {
            return left;
        }
        let op = this.advance() as SymbolToken;
        let right = this.conditionalExpression();
        if (op.value !== '=') {
            let stringOp = op.value.slice(1) as BinaryOperator;
            right = {pos: op.pos, type: 'binary-expression', op: stringOp, left, right};
        }
        return {pos: op.pos, type: 'binary-expression', op: '=', left, right};
    }

    expressionStatement(): Statement {
        let pos = this.getCurrentPosition();
        let expr = this.expression();
        this.eat(LINE_END);
        return {pos, type: 'expression-statement', expr};
    }

    blockStatement(): BlockStatement {
        let token = this.eat('{');
        let data: Statement[] = [];
        while (!this.match('}')) {
            let value = this.statement();
            if (value !== undefined) {
                data.push(value);
            }
        }
        return {pos: token.pos, type: 'block-statement', data};
    }

    rleStatement(): Statement {
        let pos = this.getCurrentPosition();
        let grid = this.expression();
        let gens: Expression | undefined = undefined;
        if (this.match(':')) {
            this.advance();
            gens = this.expression();
        }
        this.eat(':');
        this.eat('newline');
        while (this.match('newline')) {
            this.advance();
        }
        let rle = this.eat('rle');
        this.eat(LINE_END);
        return {pos, type: 'rle-statement', grid, gens, rle};
    }

    variableDeclarationStatement(): Statement {
        let token = this.eat('let');
        let mut = false;
        if (this.match('mut')) {
            this.advance();
            mut = true;
        }
        let name = this.eat('identifier').name;
        let value: Expression | undefined = undefined;
        if (this.match('=')) {
            this.advance();
            value = this.expression();
        }
        if (!mut && value === undefined) {
            this.error(undefined, 'SyntaxError', `Non-mutable variable declarations must include an initializer`);
        }
        this.eat(LINE_END);
        return {pos: token.pos, type: 'variable-declaration-statement', mut, name, value};
    }

    ifStatement(): Statement {
        let token = this.eat('if');
        let condition = this.expression();
        let ifTrue = this.blockStatement();
        let ifFalse: BlockStatement | undefined = undefined;
        if (this.match('else')) {
            this.advance();
            ifFalse = this.blockStatement();
        }
        return {pos: token.pos, type: 'if-statement', condition, ifTrue, ifFalse};
    }

    forStatement(): Statement {
        let token = this.eat('for');
        let mut = false;
        if (this.match('mut')) {
            this.advance();
            mut = true;
        }
        let name = this.eat('identifier');
        this.eat('identifier in');
        let iterateOver = this.expression();
        let value = this.blockStatement();
        return {pos: token.pos, type: 'for-statement', mut, name, iterateOver, value};
    }

    whileStatement(): Statement {
        let token = this.eat('while');
        let condition = this.expression();
        let value = this.blockStatement();
        return {pos: token.pos, type: 'while-statement', condition, value};
    }

    statement(): Statement | undefined {
        while (this.match(LINE_END) && !this.match(EOF)) {
            this.advance();
        }
        if (this.match(EOF)) {
            return undefined;
        }
        let out: Statement;
        if (this.match('{')) {
            out = this.blockStatement();
        } else if (this.match('rle')) {
            out = this.rleStatement();
        } else if (this.match('let')) {
            out = this.variableDeclarationStatement();
        } else if (this.match('if')) {
            out = this.ifStatement();
        } else if (this.match('for')) {
            out = this.forStatement();
        } else if (this.match('while')) {
            out = this.whileStatement();
        } else {
            // advanced lookahead to see if it is a RLE statement
            // this works because RLE statements must end in a colon followed
            // by one or more newlines and then a RLE token, and this is
            // always an invalid expression, so the grammar is unambiguous
            let startPos = this.pos;
            let isRLEStatement = false;
            while (!this.match(LINE_END)) {
                if (this.match(':')) {
                    this.advance();
                    while (this.match(LINE_END) && !this.match(EOF)) {
                        this.advance();
                    }
                    if (this.match('rle')) {
                        isRLEStatement = true;
                    }
                    break;
                } else {
                    this.advance();
                }
            }
            this.pos = startPos;
            if (isRLEStatement) {
                out = this.rleStatement();
            } else {
                out = this.expressionStatement();
            }
        }
        while (this.match(LINE_END) && !this.match(EOF)) {
            this.advance();
        }
        return out;
    }

    async program(): Promise<Statement[]> {
        let out: Statement[] = [];
        while (this.pos < this.tokens.length) {
            let value = this.statement();
            if (value !== undefined) {
                out.push(value);
            }
        }
        return out;
    }

}


interface Variable {
    mut?: boolean;
    value: Value;
}

class Scope {

    runner: XLSScriptRunner;
    parent: Scope | undefined;
    vars: Map<string, Variable>;
    states: Map<number, FullCellSpecifier>;

    constructor(runner: XLSScriptRunner, parent: Scope | undefined) {
        this.runner = runner;
        this.parent = parent;
        this.vars = new Map();
        this.states = new Map();
    }

    getVar(pos: HasPos, name: string): Variable {
        let value = this.vars.get(name);
        if (value !== undefined) {
            return value;
        } else if (this.parent) {
            return this.parent.getVar(pos, name);
        } else {
            let value = BUILTIN_GLOBALS.get(name);
            if (value !== undefined) {
                return {mut: false, value};
            } else {
                this.runner.error(pos, 'NameError', `Variable ${name} is not defined`);
            }
        }
    }

    declareVar(name: string, value: Variable): void {
        this.vars.set(name, value);
    }

    setVar(pos: HasPos, name: string, value: Value): void {
        let data = this.vars.get(name);
        if (data !== undefined) {
            if (!data.mut) {
                this.runner.error(pos, 'NameError', `Variable ${name} is not mutable`);
            }
            this.vars.set(name, {mut: data.mut, value});
        } else if (this.parent) {
            this.parent.setVar(pos, name, value);
        } else {
            this.runner.error(pos, 'NameError', `Variable ${name} is not defined`);
        }
    }

    getState(pos: HasPos, state: number): FullCellSpecifier {
        let value = this.states.get(state);
        if (value !== undefined) {
            return value;
        } else if (this.parent) {
            return this.parent.getState(pos, state);
        } else {
            this.runner.error(pos, 'NameError', `State ${state} is not defined`);
        }
    }

    setState(state: number, value: FullCellSpecifier): void {
        this.states.set(state, value);
    }

}


class XLSScriptRunner {

    readonly ulp: ULP;

    code: string;
    background: Grid;
    scope: Scope;
    callStack: {name: string, pos: Position}[];

    constructor(code: string, ulp: ULP) {
        this.code = code;
        this.ulp = ulp;
        this.background = new Grid();
        this.scope = new Scope(this, undefined);
        this.callStack = [];
        this.scope.declareVar('grid', {mut: false, value: {pos: BUILTIN_POS, type: 'grid', grid: ulp.grid}});
    }

    error(pos: HasPos, type: string, msg: string): never {
        if ('pos' in pos) {
            pos = pos.pos;
        }
        let text = `${type}: ${msg}\n    at ${positionToString(pos)}`;
        if (this.callStack) {
            for (let value of this.callStack) {
                text += `\n    at ${value.name} at ${positionToString(value.pos)}`;
            }
        }
        throw new XLSScriptError(text);
    }

    pushScope(): void {
        this.scope = new Scope(this, this.scope);
    }

    popScope(): void {
        if (this.scope.parent === undefined) {
            throw new Error(`This error should not occur (cannot pop scope)`);
        }
        this.scope = this.scope.parent;
    }

    _currentDefaultRuleFunc: ClauseFunc | undefined = undefined;

    getCurrentDefaultRuleFunc(pos: HasPos): ClauseFunc {
        if (this._currentDefaultRuleFunc === undefined) {
            this.error(pos, 'ValueError', `No rule is currently set`);
        } else {
            return this._currentDefaultRuleFunc;
        }
    }

    wrappedReassign(pos: HasPos, from: Cell, to: Cell, coord: Coord): void {
        try {
            this.ulp.reassign(from, to);
        } catch (error) {
            if (error instanceof XLSError && error.message === 'Contradiction detected') {
                this.error(pos, 'ValueError', `Contradiction detected while trying to set cell at ${coord} from value ${from} to ${to}`);
            }
        }
    }

    assertType<T extends ValueType>(value: Value, type: T, whatFor: string): ValueOfType<T> {
        if (value.type !== type) {
            this.error(value, 'TypeError', `Invalid type for ${whatFor}: '${value.type}' (expected type '${type}')`);
        }
        return value as ValueOfType<T>;
    }

    castToCellSpecifier(value: Value, whatFor: string): CellSpecifier {
        if (value.type === 'number') {
            if (!Number.isInteger(value) || value.value < 0 || value.value >= this.ulp.maxStates) {
                this.error(value, 'ValueError', `Invalid number for conversion to cell specifier: '${value}'`);
            }
            let cell = new GridCell(value.value, this.getCurrentDefaultRuleFunc(value));
            return {pos: value.pos, type: 'cell_specifier', kind: 'cell', cell};
        } else if (value.type === 'cell_specifier') {
            return value;
        } else {
            this.error(value, 'TypeError', `Invalid type for ${whatFor}: '${value.type}' (expected type 'number' or 'cell_specifier')`);
        }
    }

    resolveGeneration(pos: HasPos, value: number): number;
    resolveGeneration(pos: HasPos, value: number, isRelative: boolean, relativeTo: number): number;
    resolveGeneration(pos: HasPos, value: number, isRelative: boolean = false, relativeTo: number = 0): number {
        let grid = this.ulp.grid;
        let out = isRelative ? value + relativeTo : value;
        if (out < 0) {
            out += grid.gens;
        }
        if (out < 0 || out >= grid.gens) {
            this.error(pos, 'ValueError', `Generation out of bounds: '${out}' (expected -${grid.gens} to ${grid.gens - 1})`);
        }
        return out;
    }

    setCells(pos: HasPos, grid: GridLike, gens: number[], x: number, y: number, value: CellSpecifier, baseT: number): void {
        if (value.kind === 'periodic') {
            // special case for periodic
            let vars: Cell[] = [];
            for (let i = 0; i < value.period; i++) {
                vars.push(this.ulp.getNewVar());
            }
            for (let t of gens) {
                let cell = vars[(t + baseT) % vars.length];
                let old = grid.get(t, x, y);
                old.func = value.func;
                if (old.value === -1) {
                    old.value = cell;
                } else {
                    this.wrappedReassign(pos, old.value, cell, new Coord(t, x, y));
                }
            }
            return;
        }
        for (let t of gens) {
            if (value.kind === 'nop') {
                if (value.func !== undefined) {
                    grid.get(t, x, y).func = value.func;
                }
            } else if (value.kind === 'cell') {
                let old = grid.get(t, x, y);
                old.func = value.cell.func;
                if (old.value === -1) {
                    old.value = value.cell.value;
                } else {
                    this.wrappedReassign(pos, old.value, value.cell.value, new Coord(t, x, y));
                }
            } else {
                throw new Error(`This error should not occur (invalid cell specifier: ${value satisfies never})`);
            }
        }
    }

    valueToString(value: Value): string {
        if (value.type === 'null') {
            return `null`;
        } else if (value.type === 'boolean') {
            return value.value ? `true` : `false`;
        } else if (value.type === 'number') {
            return String(value);
        } else if (value.type === 'string') {
            return getStringValue(value);
        } else if (value.type === 'range') {
            let start = value.start === undefined ? '' : value.start;
            let end = value.end === undefined ? '' : value.end;
            return `${start}..${end}`;
        } else if (value.type === 'vector') {
            return `(${value.data.join(', ')})`;
        } else if (value.type === 'vector_range') {
            return '(' + value.data.map(item => {
                let start = item[0] === undefined ? '' : item[0];
                let end = item[1] === undefined ? '' : item[1];
                return `${start}..${end}`;
            }).join(', ') + ')';
        } else if (value.type === 'cell_specifier') {
            if (value.kind === 'nop') {
                return `nop`;
            } else if (value.kind === 'cell') {
                let cell = value.cell.value;
                if (cell === OFF) {
                    return `off`;
                } else if (cell === ON) {
                    return `on`;
                } else if (cell === UNKNOWN) {
                    return `unknown`;
                } else {
                    return `var(${cell})`;
                }
            } else if (value.kind === 'periodic') {
                return `period ${value.period}`;
            } else {
                throw new Error(`This error should not occur (invalid cell specifier: ${value satisfies never})`);
            }
        } else if (value.type === 'full_cell_specifier') {
            return `<full cell specifier (go make speedy improve this if you want it better)>`;
        } else if (value.type === 'grid') {
            return `<grid (go make speedy improve this if you want it better)>`;
        } else if (value.type === 'grid_slice') {
            return `<grid slice (go make speedy improve this if you want it better)>`;
        } else if (value.type === 'function') {
            let boundText = (value.boundArgs && value.boundArgs.length > 0) ? 'bound ' : '';
            if (value.builtin) {
                return `<${boundText}builtin function ${value.name}>`;
            } else {
                return `<${boundText}function ${value.name}>`;
            }
        } else if (value.type === 'reference') {
            return `<reference (go make speedy improve this if you want it better)>`;
        } else {
            throw new Error(`This error should not occur (invalid value: ${value satisfies never})`);
        }
    }

    getLvalue(value: Lvalue): Value {
        if (value.kind === 'identifier') {
            return this.scope.getVar(value, value.name).value;
        } else if (value.kind === 'state') {
            return this.scope.getState(value, value.value);
        } else if (value.kind === 'grid_coord') {
            if (!value.grid.isInBounds(value.coord)) {
                this.error(value, 'ValueError', `Coord is out of bounds: ${value.coord}`);
            } else {
                return {pos: value.pos, type: 'cell_specifier', kind: 'cell', cell: value.grid.get(value.coord)};
            }
        } else if (value.kind === 'grid_magic_key') {
            let pos = value.pos;
            if (value.key === 'width') {
                return {pos, type: 'number', value: value.grid.width};
            } else if (value.key === 'height') {
                return {pos, type: 'number', value: value.grid.height};
            } else if (value.key === 'gens') {
                return {pos, type: 'number', value: value.grid.gens};
            } else {
                throw new Error(`This error should not occur (invalid grid key: ${value.key satisfies never})`);
            }
        } else {
            throw new Error(`This error should not occur (invalid lvalue: ${value satisfies never})`);
        }
    }

    setLvalue(pos: HasPos, left: Lvalue, right: Value): void {
        // setSpecialIdentifier(opPos: HasPos, name: SpecialIdentifier, value: Value): void {
        //     let grid = this.ulp.grid;
        //     let noclause = this.noclause;
        //     if (name === 'width' || name === 'height' || name === 'gens') {
        //         value = this.assertType(opPos, value, 'number', `assignment to ${name}`);
        //         if (name === 'width') {
        //             if (grid.width === value) {
        //                 return;
        //             } else if (value < grid.width) {
        //                 grid.shrink({right: grid.width - value});
        //                 noclause.shrink({right: grid.width - value});
        //             } else {
        //                 grid.expand({right: value - grid.width}, this.ulp.getNewVar.bind(this.ulp));
        //                 noclause.expand({right: value - grid.width}, 0);
        //             }
        //         } else if (name === 'height') {
        //             if (grid.height === value) {
        //                 return;
        //             } else if (value < grid.height) {
        //                 grid.shrink({bottom: grid.height - value});
        //                 noclause.shrink({right: grid.width - value});
        //             } else {
        //                 grid.expand({bottom: value - grid.height}, this.ulp.getNewVar.bind(this.ulp));
        //                 noclause.expand({right: value - grid.width}, 0);
        //             }
        //         } else {
        //         }
        //     } else {
        //         throw new Error(`This error should not occur (invalid special identifier: ${name satisfies never})`);
        //     }
        // }
        if (left.kind === 'identifier') {
            left.scope.setVar(left, left.name, right);
        } else if (left.kind === 'state') {
            left.scope.setState(left.value, this.assertType(right, 'full_cell_specifier', `assigning to state`));
        } else if (left.kind === 'grid_coord') {
            let coord = left.coord;
            if (!left.grid.isInBounds(coord)) {
                this.error(left, 'ValueError', `Coord is out of bounds: ${coord}`);
            } else {
                right = this.castToCellSpecifier(right, `assignment to grid`);
                this.setCells(pos, left.grid, [coord.t], coord.x, coord.y, right, 0);
            }
        } else if (left.kind === 'grid_magic_key') {
            if (left.key === 'width' || left.key === 'height' || left.key === 'gens') {
                let grid = left.grid;
                let value = this.assertType(right, 'number', `assignment to grid.${left.key}`).value;
                if (!(grid instanceof Grid)) {
                    this.error(pos, 'TypeError', `Cannot change dimensions of grid slice (the dimensions of a normal grid can be changed)`);
                }
                if (left.key === 'width') {
                    let amount = value - grid.width;
                    if (amount < 0) {
                        grid.shrink({right: -amount});
                    } else {
                        grid.expand({right: amount}, new GridCell(UNKNOWN, this.getCurrentDefaultRuleFunc(pos)));
                    }
                } else if (left.key === 'height') {
                    let amount = value - grid.height;
                    if (amount < 0) {
                        grid.shrink({bottom: -amount});
                    } else {
                        grid.expand({bottom: amount}, new GridCell(UNKNOWN, this.getCurrentDefaultRuleFunc(pos)));
                    }
                } else {
                    let amount = value - grid.gens;
                    if (amount < 0) {
                        grid.shrink({end: -amount});
                    } else {
                        grid.expand({end: amount}, new GridCell(UNKNOWN, this.getCurrentDefaultRuleFunc(pos)));
                    }
                }
            }
        } else {
            throw new Error(`This error should not occur (invalid lvalue: ${left satisfies never})`);
        }
    }

    decayLvalues(value: ExpressionValue): Value {
        if (isLvalue(value)) {
            return this.getLvalue(value);
        } else {
            return value;
        }
    }

    vectorLiteralExpression(node: VectorLiteralExpression): ExpressionValue {
        // todo: make this work with ranges
        let data = node.data.map(value => this.expression(value));
        if (data.every(value => value.type === 'number')) {
            return {pos: node.pos, type: 'vector', data: data.map(x => x.value)};
        } else {
            let out: [number | undefined, number | undefined][] = [];
            for (let value of data) {
                if (value.type === 'number') {
                    out.push([value.value, value.value]);
                } else if (value.type === 'range') {
                    out.push([value.start, value.end]);
                } else {
                    this.error(value, 'TypeError', `Invalid type for vector literal element: '${value.type}' (expected 'number' or 'range')`);
                }
            }
            return {pos: node.pos, type: 'vector_range', data: out};
        }
    }

    assertArgType<T extends ValueType>(value: Value | undefined, type: T, paramName: string): ValueOfType<T> {
        let {name, pos} = this.callStack[this.callStack.length - 1];
        if (value === undefined) {
            this.error(pos, 'TypeError', `No argument passed for parameter ${paramName} of builtin function ${name}`);
        }
        if (value.type !== type) {
            this.error(value, 'TypeError', `Invalid type for parameter ${paramName} of builtin function ${name}: '${value.type}' (expected type '${type}')`);
        }
        return value as ValueOfType<T>;
    }

    functionCallExpression(node: FunctionCallExpression): ExpressionValue {
        let func = this.assertType(this.expression(node.func), 'function', `for calling`);
        let args = [];
        if (func.boundArgs !== undefined) {
            for (let arg of func.boundArgs) {
                args.push(arg);
            }
        }
        for (let arg of node.args) {
            args.push(this.expression(arg));
        }
        if (func.builtin) {
            this.callStack.push({name: func.name, pos: node.pos});
            let out = func.func.call(this, node.pos, ...args);
            this.callStack.pop();
            return out;
        } else {
            throw new Error(`Cannot call non-builtin functions yet`);
        }
    }

    memberExpression(node: MemberExpression): ExpressionValue {
        let pos = node.pos;
        let value = this.expression(node.value);
        let member: string | NumberValue | Vector | Range | VectorRange;
        if (node.member.type === 'identifier') {
            member = node.member.name;
        } else {
            let value = this.expression(node.member);
            if (value.type === 'string') {
                member = getStringValue(value);
            } else if (value.type === 'number' || value.type === 'vector' || value.type === 'range' || value.type === 'vector_range') {
                member = value;
            } else {
                this.error(node, 'TypeError', `Invalid type for member: '${value.type}' (expected 'string', 'number', 'vector', 'range', or 'vector_range)`);
            }
        }
        let memberStr = typeof member === 'string' ? member : this.valueToString(member);
        if (value.type === 'null') {
            this.error(node, 'TypeError', `Null does not have members`);
        } else if (value.type === 'vector') {
            let out: number | undefined;
            if (typeof member === 'string') {
                if (member === 'length') {
                    out = value.data.length;
                }
                if (value.data.length === 2) {
                    if (member === 'x') {
                        out = value.data[0];
                    } else if (member === 'y') {
                        out = value.data[1];
                    }
                } else if (value.data.length === 3) {
                    if (member === 't') {
                        out = value.data[0];
                    } else if (member === 'x') {
                        out = value.data[1];
                    } else if (member === 'y') {
                        out = value.data[2];
                    }
                }
            } else if (member.type === 'number') {
                out = value.data[member.value];
                if (out === undefined) {
                    this.error(node, 'MemberError', `Invalid vector index: '${memberStr}'`);
                }
            }
            if (out !== undefined) {
                return {pos, type: 'number', value: out};
            } else {
                if (member === 't') {
                    this.error(node, 'MemberError', `Member '${memberStr}' does not exist in vector of length ${value.data.length} (only exists in vectors of length 3)`);
                } else if (member === 'x' || member === 'y') {
                    this.error(node, 'MemberError', `Member '${memberStr}' does not exist in vector of length ${value.data.length} (only exists in vectors of length 2 or 3)`);
                }
            }
        } else if (value.type === 'grid' || value.type === 'grid_slice') {
            let grid = value.grid;
            if (typeof member === 'string') {
                if (GRID_MAGIC_KEYS.has(member as GridMagicKey)) {
                    return {pos, type: 'lvalue', kind: 'grid_magic_key', grid, key: member as GridMagicKey};
                }
            } else if (member.type === 'vector') {
                if (member.data.length !== 3) {
                    this.error(node, 'MemberError', `Cannot index grid-like value with vector of length  ${member.data.length} (must be length 3)`);
                }
                let t = member.data[0];
                let x = member.data[1];
                let y = member.data[2];
                let coord = new Coord(t, x, y);
                return {pos, type: 'lvalue', kind: 'grid_coord', grid, coord};
            } else if (member.type === 'vector_range') {
                if (member.data.length !== 3) {
                    this.error(node, 'MemberError', `Cannot index grid-like value with vector of length  ${member.data.length} (must be length 3)`);
                }
                let start = new Coord(
                    member.data[0][0] ?? 0,
                    member.data[1][0] ?? 0,
                    member.data[2][0] ?? 0,
                );
                let end = new Coord(
                    member.data[0][0] ?? grid.gens,
                    member.data[1][0] ?? grid.width,
                    member.data[2][0] ?? grid.height,
                );
                let slice = new GridSlice(grid, start, end);
                return {pos, type: 'grid_slice', grid: slice};
            }
        }
        if (typeof member !== 'string') {
            this.error(node, 'MemberError', `Value of type '${value.type}' does not have members of type '${member.type}'`);
        }
        let out = BUILTIN_METHODS[value.type].get(member);
        if (out !== undefined) {
            if (out.type === 'function') {
                out = copyValue(out);
                if (!out.boundArgs) {
                    out.boundArgs = [];
                }
                out.boundArgs.push(value);
            }
            return out;
        }
        this.error(node, 'MemberError', `Member '${memberStr}' does not exist in value of type '${value.type}'`);
    }

    unaryExpression(node: UnaryExpression): ExpressionValue {
        let op = node.op;
        let arg = this._expression(node.value);
        if (op === '+') {
            let value = this.assertType(this.decayLvalues(arg), 'number', `unary + operator`).value;
            return {pos: node.pos, type: 'number', value};
        } else if (op === '-') {
            let value = -this.assertType(this.decayLvalues(arg), 'number', `unary - operator`).value;
            return {pos: node.pos, type: 'number', value};
        } else if (op === '~') {
            let value = Number(~BigInt(this.assertType(this.decayLvalues(arg), 'number', `unary ~ operator`).value));
            return {pos: node.pos, type: 'number', value};
        } else if (op === '!') {
            let value = !valueIsTruthy(this.decayLvalues(arg));
            return {pos: node.pos, type: 'boolean', value};
        } else if (op === '&') {
            if (!isLvalue(arg)) {
                this.error(arg, 'SyntaxError', `Expected lvalue for unary & operator`);
            }
            return {pos: node.pos, type: 'reference', value: arg};
        } else if (op === '*') {
            let out = this.assertType(this.decayLvalues(arg), 'reference', `unary * operator`).value;
            out = copyLvalue(out);
            out.pos = node.pos;
            return out;
        } else if (op === 'state') {
            let value = this.assertType(this.decayLvalues(arg), 'number', `state operator`).value;
            return {pos: node.pos, type: 'lvalue', kind: 'state', scope: this.scope, value};
        } else if (op === 'noclause') {
            let value = copyValue(this.assertType(this.decayLvalues(arg), 'cell_specifier', `${op} operator`));
            if (value.kind === 'cell') {
                value.cell.func = undefined;
            } else {
                value.func = undefined;
            }
            return value;
        } else if (op === 'period') {
            let period = this.assertType(this.decayLvalues(arg), 'number', `period operator`).value;
            let func = this.getCurrentDefaultRuleFunc(node);
            return {pos: node.pos, type: 'cell_specifier', kind: 'periodic', period, func};
        } else if (op === 'typeof') {
            return {pos: node.pos, type: 'string', chars: toStringChars(this.decayLvalues(arg).type)};
        } else {
            throw new Error(`This error should not occur (invalid unary operator: ${op satisfies never})`);
        }
    }

    rangeExpression(node: RangeExpression): ExpressionValue {
        let pos = node.pos;
        let start = node.start === undefined ? undefined : this.expression(node.start);
        let end = node.end === undefined ? undefined : this.expression(node.end);
        if (start === undefined) {
            if (end === undefined) {
                return {pos, type: 'range', start: undefined, end: undefined};
            } else if (end.type === 'number') {
                return {pos, type: 'range', start: undefined, end: end.value};
            } else if (end.type === 'vector') {
                return {pos, type: 'vector_range', data: end.data.map(value => [undefined, value])};
            } else {
                this.error(end, 'TypeError', `Cannot make range out of value of type '${end.type}'`);
            }
        } else if (end === undefined) {
            if (start.type === 'number') {
                return {pos, type: 'range', start: start.value, end: undefined};
            } else if (start.type === 'vector') {
                return {pos, type: 'vector_range', data: start.data.map(value => [value, undefined])};
            } else {
                this.error(start, 'TypeError', `Cannot make range out of value of type '${start.type}'`);
            }
        } else {
            if (!(start.type === 'number' || start.type === 'vector')) {
                this.error(start, 'TypeError', `Cannot make range out of value of type '${start.type}'`);
            }
            if (!(end.type === 'number' || end.type === 'vector')) {
                this.error(end, 'TypeError', `Cannot make range out of value of type '${end.type}'`);
            }
            if (start.type === 'number' && end.type === 'number') {
                return {pos, type: 'range', start: start.value, end: end.value};
            } else if (start.type === 'vector' && end.type === 'vector') {
                if (start.data.length !== end.data.length) {
                    this.error(node, 'ValueError', `Cannot make range out of vectors with two different lengths ${start.data.length} and ${end.data.length}`);
                }
                let data: [number, number][] = [];
                for (let i = 0; i < start.data.length; i++) {
                    data.push([start.data[i], end.data[i]]);
                }
                return {pos, type: 'vector_range', data};
            } else {
                this.error(node, 'TypeError', `Cannot make range out of values of two different types '${start.type}' and '${end.type}'`);
            }
        }
    }

    binaryExpression(node: BinaryExpression): ExpressionValue {
        let op = node.op;
        let left = this._expression(node.left);
        // first the assignment operator
        if (op === '=') {
            if (!isLvalue(left)) {
                this.error(left, 'SyntaxError', `Left-hand operand of assignment operator is not an lvalue`);
            }
            let right = this.expression(node.right);
            this.setLvalue(node, left, right);
            return right;
        }
        left = this.decayLvalues(left);
        // next the short-circuiting operators
        if (op === '&&') {
            let value: boolean;
            if (valueIsTruthy(left)) {
                value = valueIsTruthy(this.expression(node.right));
            } else {
                value = false;
            }
            return {pos: node.pos, type: 'boolean', value};
        } else if (op === '||') {
            let value: boolean;
            if (valueIsTruthy(left)) {
                value = true;
            } else {
                value = valueIsTruthy(this.expression(node.right));;
            }
            return {pos: node.pos, type: 'boolean', value};
        } else if (op === '??') {
            if (left === null) {
                return this.expression(node.right);
            } else {
                return left;
            }
        }
        // then the operators that don't work on only numbers
        let right = this.expression(node.right);
        if (op === '==') {
            return {pos: node.pos, type: 'boolean', value: valuesAreEqual(left, right)};
        } else if (op === '!=') {
            return {pos: node.pos, type: 'boolean', value: !valuesAreEqual(left, right)};
        } else if (op === '^^') {
            return {pos: node.pos, type: 'boolean', value: valueIsTruthy(left) !== valueIsTruthy(right)};
        }
        // finally the operators that work on numbers
        let x = this.assertType(left, 'number', `${op} operator`).value;
        let y = this.assertType(right, 'number', `${op} operator`).value;
        let out: number;
        if (op === '+') {
            out = x + y;
        } else if (op === '-') {
            out = x - y;
        } else if (op === '*') {
            out = x * y;
        } else if (op === '/') {
            out = x / y;
        } else if (op === '%') {
            out = x % y;
        } else if (op === '<<') {
            out = Number(BigInt(x) << BigInt(y));
        } else if (op === '>>') {
            out = Number(BigInt(x) >> BigInt(y));
        } else if (op === '&') {
            out = Number(BigInt(x) & BigInt(y));
        } else if (op === '^') {
            out = Number(BigInt(x) ^ BigInt(y));
        } else if (op === '|') {
            out = Number(BigInt(x) | BigInt(y));
        } else if (op === '<') {
            return {pos: node.pos, type: 'boolean', value: x < y};
        } else if (op === '<=') {
            return {pos: node.pos, type: 'boolean', value: x <= y};
        } else if (op === '>') {
            return {pos: node.pos, type: 'boolean', value: x > y};
        } else if (op === '>=') {
            return {pos: node.pos, type: 'boolean', value: x >= y};
        } else {
            throw new Error(`This error should not occur (invalid binary operator: ${op satisfies never})`);
        }
        return {pos: node.pos, type: 'number', value: out};
    }

    fullCellSpecifierExpression(node: FullCellSpecifierExpression): Value {
        let parts: FullCellSpecifierPart[] = [];
        for (let part of node.parts) {
            let gens = this.expression(part.gens);
            if (!(gens.type === 'number' || gens.type === 'range')) {
                this.error(gens, 'TypeError', `Invalid type for cell specifier generation: '${gens.type}' (expected 'number' or 'range')`);
            }
            let specifier = this.castToCellSpecifier(this.expression(part.value), `full cell specifier part`);
            parts.push({isRelative: part.isRelative, gens, value: specifier});
        }
        return {pos: node.pos, type: 'full_cell_specifier', parts};
    }

    _expression(node: Expression): ExpressionValue {
        if (node.type === 'identifier-expression') {
            return {pos: node.pos, type: 'lvalue', kind: 'identifier', scope: this.scope, name: node.name};
        } else if (node.type === 'simple-literal-expression') {
            return node.value;
        } else if (node.type === 'cell-literal-expression') {
            let cell = new GridCell(node.value, this.getCurrentDefaultRuleFunc(node));
            return {pos: node.pos, type: 'cell_specifier', kind: 'cell', cell};
        } else if (node.type === 'vector-literal-expression') {
            return this.vectorLiteralExpression(node);
        } else if (node.type === 'function-call-expression') {
            return this.functionCallExpression(node);
        } else if (node.type === 'member-expression') {
            return this.memberExpression(node);
        } else if (node.type === 'unary-expression') {
            return this.unaryExpression(node);
        } else if (node.type === 'range-expression') {
            return this.rangeExpression(node);
        } else if (node.type === 'binary-expression') {
            return this.binaryExpression(node);
        } else if (node.type === 'full-state-specifier-expression') {
            return this.fullCellSpecifierExpression(node);
        } else if (node.type === 'conditional-expression') {
            if (valueIsTruthy(this.expression(node.condition))) {
                return this._expression(node.ifTrue);
            } else {
                return this._expression(node.ifFalse);
            }
        } else {
            throw new Error(`This error should not occur (invalid expression: ${node satisfies never})`);
        }
    }
    
    expression(node: Expression): Value {
        return this.decayLvalues(this._expression(node));
    }

    blockStatement(node: BlockStatement): void {
        this.pushScope();
        for (let value of node.data) {
            this.statement(value);
        }
        this.popScope();
    }

    rleStatement(node: RLEStatement): void {
        // get the grid to set
        let gridValue = this.expression(node.grid);
        if (!(gridValue.type === 'grid' || gridValue.type === 'grid_slice')) {
            this.error(gridValue, 'TypeError', `Invalid type for RLE statement grid: '${gridValue.type}' (expected type 'grid' or 'grid_slice')`);
        }
        let grid = gridValue.grid;
        // get the generations to set in the grid
        let baseGens: number[];
        if (node.gens) {
            let value = this.expression(node.gens);
            if (value.type === 'number') {
                baseGens = [this.resolveGeneration(value, value.value)];
            } else if (value.type === 'range') {
                let start = value.start ?? 0;
                let end = value.end ?? grid.gens;
                baseGens = [];
                for (let i = start; i < end; i++) {
                    baseGens.push(this.resolveGeneration(value, i));
                }
            } else {
                this.error(value, 'TypeError', `Invalid type for gens value of RLE statement: '${value.type}' (expected type 'number' or 'range')`);
            }
        } else {
            baseGens = [];
            for (let i = 0; i < grid.gens; i++) {
                baseGens.push(i);
            }
        }
        // get the pattern corresponding to the RLE
        let rle = node.rle.value;
        let p: IdentityPattern;
        try {
            p = IdentityPattern.loadRLE(rle);
        } catch (error) {
            if (error instanceof LifewebError) {
                this.error(node.rle, 'LifewebError', error.message);
            } else {
                throw error;
            }
        }
        // expand the pattern by the width and height given in the RLE header
        let header = rle.slice(0, rle.indexOf('\n'));
        let match = header.match(/^x\s*=\s*(\d+)\s*,\s*y\s*=\s*(\d+)/);
        if (!match) {
            throw new Error(`This error should not occur (bad RLE header)`);
        }
        let width = Math.max(Number(match[1]), p.width);
        let height = Math.max(Number(match[2]), p.height);
        p.expand(0, height - p.height, 0, width - p.width);
        // expand the grid to fit the pattern
        if (width > grid.width || height > grid.height) {
            if (grid instanceof GridSlice) {
                this.error(gridValue, 'TypeError', `RLE too big for grid slice (grid slices are not expandable, normal grids are)`);
            }
            let cell = new GridCell(UNKNOWN, this.getCurrentDefaultRuleFunc(node.rle));
            if (width > grid.width) {
                grid.expand({right: width - grid.width}, cell);
            }
            if (height > grid.height) {
                grid.expand({bottom: width - grid.height}, cell);
            }
        }
        for (let y = 0; y < p.height; y++) {
            for (let x = 0; x < p.width; x++) {
                let state = p.get(x, y);
                for (let part of this.scope.getState(node.rle, state).parts) {
                    for (let baseGen of baseGens) {
                        let gens: number[];
                        if (part.gens.type === 'number') {
                            gens = [baseGen];
                        } else {
                            let start = part.gens.start ?? 0;
                            let end = part.gens.end ?? grid.gens;
                            gens = [];
                            for (let i = start; i < end; i++) {
                                gens.push(i);
                            }
                        }
                        if (part.isRelative) {
                            gens = gens.map(gen => gen + baseGen); 
                        }
                        gens = gens.map(gen => this.resolveGeneration(node.rle, gen));
                        this.setCells(node.rle, grid, gens, x, y, part.value, baseGen);
                    }
                }
            }
        }
    }

    variableDeclarationStatement(node: VariableDeclarationStatement): void {
        let value: Value;
        if (node.value !== undefined) {
            value = this.expression(node.value);
        } else {
            value = {pos: node.pos, type: 'null'};
        }
        let data: Variable = {mut: node.mut, value};
        this.scope.declareVar(node.name, data);
    }

    ifStatement(node: IfStatement): void {
        if (valueIsTruthy(this.expression(node.condition))) {
            this.blockStatement(node.ifTrue);
        } else {
            if (node.ifFalse) {
                this.blockStatement(node.ifFalse);
            }
        }
    }

    forStatement(node: ForStatement): void {
        let mut = node.mut;
        let name = node.name;
        let value = this.expression(node.iterateOver);
        if (value.type === 'range') {
            if (value.start === undefined || value.end === undefined) {
                this.error(value, 'ValueError', `Cannot iterate over incomplete range '${this.valueToString(value)}'`);
            }
            for (let i = value.start; i < value.end; i++) {
                this.pushScope();
                let value: NumberValue = {pos: name.pos, type: 'number', value: i};
                this.scope.declareVar(name.name, {mut, value});
                this.blockStatement(node.value);
                this.popScope();
            }
        } else if (value.type === 'vector_range') {
            let data = value.data as [number, number][];
            if (data.some(item => item[0] === undefined || item[1] === undefined)) {
                this.error(value, 'ValueError', `Cannot iterate over partial range`);
            }
            let current: number[] = [];
            for (let i = 0; i < value.data.length; i++) {
                current.push(data[i][0]);
            }
            while (true) {
                let done = false;
                for (let i = data.length - 1; i >= 0; i--) {
                    current[i]++;
                    if (current[i] >= data[i][1]) {
                        if (i === 0) {
                            done = true;
                            break;
                        } else {
                            current[i] = data[i][0];
                            continue;
                        }
                    } else {
                        break;
                    }
                }
                if (done) {
                    break;
                }
                this.pushScope();
                let value: Vector = {pos: node.name.pos, type: 'vector', data: structuredClone(current)};
                this.scope.declareVar(name.name, {mut, value});
                this.blockStatement(node.value);
                this.popScope();
            }
        } else if (value.type === 'grid' || value.type === 'grid_slice') {
            for (let coord of value.grid.coords()) {
                this.pushScope();
                let value: Vector = {pos: node.name.pos, type: 'vector', data: [coord.t, coord.x, coord.y]};
                this.scope.declareVar(name.name, {mut, value});
                this.blockStatement(node.value);
                this.popScope();
            }
        } else {
            this.error(value, 'TypeError', `Invalid type for for statement: '${value.type}' (expected 'range', 'vector_range', 'grid', or 'grid_slice')`);
        }
        this.popScope();
    }

    whileStatement(node: WhileStatement): void {
        while (valueIsTruthy(this.expression(node.condition))) {
            this.blockStatement(node.value);
        }
    }

    statement(node: Statement): void {
        if (node.type === 'expression-statement') {
            this.expression(node.expr);
        } else if (node.type === 'block-statement') {
            this.blockStatement(node);
        } else if (node.type === 'rle-statement') {
            this.rleStatement(node);
        } else if (node.type === 'variable-declaration-statement') {
            this.variableDeclarationStatement(node);
        } else if (node.type === 'if-statement') {
            this.ifStatement(node);
        } else if (node.type === 'for-statement') {
            this.forStatement(node);
        } else if (node.type === 'while-statement') {
            this.whileStatement(node);
        } else {
            throw new Error(`This error should not occur (invalid statement: ${node satisfies never})`);
        }
    }

    async program(data: Statement[]): Promise<void> {
        for (let node of data) {
            this.statement(node);
        }
        let grid = this.ulp.grid;
        let bg = this.background.copy();
        // make them all noclause
        for (let cell of bg.cells()) {
            cell.func = undefined;
        }
        // expand the grid to add in the background
        let empty = new GridCell(OFF, undefined);
        grid.expand({top: 1, bottom: 1, left: 1, right: 1}, empty.copy());
        for (let t = 0; t < grid.gens; t++) {
            grid.set(t, 0, 0, bg.getAllowOOB(0, grid.width - 1, grid.height - 1) ?? empty.copy());
            grid.set(t, grid.width - 1, grid.height - 1, bg.getAllowOOB(0, 0, 0) ?? empty.copy());
            for (let x = 1; x < grid.width - 1; x++) {
                grid.set(t, x, 0, bg.getAllowOOB(t, x, grid.height - 1) ?? empty.copy());
                grid.set(t, x, grid.height - 1, bg.getAllowOOB(t, x, 0) ?? empty.copy());
            }
            for (let y = 1; y < grid.height - 1; y++) {
                grid.set(t, 0, y, bg.getAllowOOB(t, grid.width - 1, y) ?? empty.copy());
                grid.set(t, grid.width - 1, y, bg.getAllowOOB(t, 0, y) ?? empty.copy());
            }
        }
        this.ulp.addRuleClauses();
    }

}


const BUILTIN_GLOBALS = new Map<string, Value>();

const BUILTIN_METHODS: {[K in Exclude<ValueType, 'null'>]: Map<string, Value>} = {
    'boolean': new Map(),
    'number': new Map(),
    'string': new Map(),
    'range': new Map(),
    'vector': new Map(),
    'vector_range': new Map(),
    'cell_specifier': new Map(),
    'full_cell_specifier': new Map(),
    'grid': new Map(),
    'grid_slice': new Map(),
    'function': new Map(),
    'reference': new Map(),
};

function addBuiltinFunction(to: 'global' | Exclude<ValueType, 'null'> | Exclude<ValueType, 'null'>[], name: string, params: string[], func: BuiltinFunctionFunc): void {
    let value: FunctionValue = {pos: BUILTIN_POS, type: 'function', name, builtin: true, params, func};
    if (to === 'global') {
        BUILTIN_GLOBALS.set(name, value);
    } else if (Array.isArray(to)) {
        for (let type of to) {
            let value2 = copyValue(value);
            value2.name = `${type}.${value2.name}`;
            BUILTIN_METHODS[type].set(name, value);
        }
    } else {
        value.name = `${to}.${name}`;
        BUILTIN_METHODS[to].set(name, value);
    }
}


// global functions

addBuiltinFunction('global', 'set_rule', ['rule'], function(pos: Position, _rule?: Value): Value {
    let rule = this.assertArgType(_rule, 'string', 'rule');
    let p = createPattern(getStringValue(rule));
    let trs: Uint8Array;
    if (p instanceof MAPPattern) {
        trs = p.trs;
    } else if (p instanceof MAPB0Pattern) {
        trs = p.evenTrs.map(x => x === 0 ? 1 : 0);
    } else {
        this.error(rule, 'ValueError', `Rules that are not 2-state range-1 Moore-neighborhood are not supported`);
    }
    let func = this.ulp.addLookupTableClauseFunc(10);
    for (let i = 0; i < 1024; i++) {
        func.table[i] = (trs[i >> 1] === (i & 1)) ? 0 : 1;
    }
    this._currentDefaultRuleFunc = func;
    return {pos, type: 'null'};
});


// grid methods

addBuiltinFunction('grid', 'apply_wrap', ['dx', 'dy'], function(pos: Position, _grid?: Value, _dx?: Value, _dy?: Value): Value {
    let grid = this.assertArgType(_grid, 'grid', 'grid').grid;
    let dx = this.assertArgType(_dx, 'number', 'dx').value;
    let dy = this.assertArgType(_dy, 'number', 'dy').value;
    let ruleFunc = this.getCurrentDefaultRuleFunc(pos);
    grid.expand({end: 1}, new GridCell(UNKNOWN, ruleFunc));
    for (let endY = 0; endY < grid.height; endY++) {
        for (let endX = 0; endX < grid.width; endX++) {
            let startX = endX - dx;
            let startY = endY - dy;
            if (!grid.isInBounds(0, startX, startY)) {
                // this cell doesn't exist at the start so it must be 0
                grid.set(grid.gens - 1, endX, endY, new GridCell(OFF, ruleFunc));
            } else {
                // bind the start and end together
                this.ulp.reassign(grid.get(0, startX, startY).value, grid.get(grid.gens - 1, endX, endY).value);
            }
        }
    }
    for (let startY = 0; startY < grid.height; startY++) {
        for (let startX = 0; startX < grid.width; startX++) {
            let endX = startX + dx;
            let endY = startY + dy;
            if (endX < 0 || endX >= grid.width || endY < 0 || endY >= grid.height) {
                // this cell doesn't exist at the end so it must be 0
                grid.set(0, startX, startY, new GridCell(OFF, ruleFunc));
            }
        }
    }
    return {pos, type: 'null'};
});


export async function runScript(code: string, filePath: string = '<anonymous>'): Promise<ULP> {
    let ulp = new ULP(2);
    let parser = new XLSScriptParser(filePath, code, ulp);
    let data = await parser.program();
    let runner = new XLSScriptRunner(code, ulp);
    await runner.program(data);
    ulp.normalize();
    return ulp;
}

export async function runScriptFromFile(filePath: string): Promise<ULP> {
    let fs = await import('node:fs/promises');
    let code = (await fs.readFile(filePath)).toString();
    return await runScript(code, filePath);
}
