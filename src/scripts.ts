
import {LifewebError, IdentityPattern} from '../lifeweb/lib/index.js';
import {XLSError, Cell, OFF, ON, Grid, ULP} from './base.js';


interface Position {
    file: string;
    line: number;
    col: number;
}

function positionToString(pos: Position): string {
    return `${pos.file}:${pos.line}:${pos.col}`;
}

type ObjWithPos = {pos: Position};

type HasPos = Position | ObjWithPos;

class XLSScriptError extends XLSError {

    name: string = 'XLSScriptError';
    [Symbol.toStringTag]: string = 'XLSScriptError';

}


type Generation = {type: 'generation', isAbsolute: boolean, value: number};
type Coord = {type: 'coord', t: Generation, x: number, y: number};

type Scalar = number | Generation | Coord;
type Range<T extends Scalar = Scalar> = {type: 'range', isInclusive: boolean, start: T, end: T};

type StateSpecifier = {type: 'state_specifier'} & (
    | {kind: 'nop'}
    | {kind: 'cell', cell: 'off' | 'on' | 'unknown'}
    | {kind: 'var', number: number}
    | {kind: 'periodic', period: number}
    | {kind: 'raw', value: Cell}
) & {noClause?: boolean};

function stateSpecifiersAreEqual(x: StateSpecifier, y: StateSpecifier): boolean {
    if (x.kind !== y.kind || x.noClause !== y.noClause) {
        return false;
    }
    if (x.kind === 'nop' && y.kind === 'nop') {
        return true;
    } else if (x.kind === 'cell' && y.kind === 'cell') {
        return x.cell === y.cell;
    } else if (x.kind === 'periodic' && y.kind === 'periodic') {
        return x.period === y.period;
    } else if (x.kind === 'raw' && y.kind === 'raw') {
        return x.value === y.value;
    } else {
        throw new Error(`This error should not occur (invalid state specifier)`);
    }
}

type FullStateSpecifierPart = {gens: Generation | Range<Generation>, value: StateSpecifier};
type FullStateSpecifier = {type: 'full_state_specifier', parts: FullStateSpecifierPart[]};

type Value = null | boolean | number | Generation | Coord | Range | StateSpecifier | FullStateSpecifier;

type ValueType = 'null' | 'boolean' | 'number' | Extract<Value, object>['type'];

type ValueOfType<T extends ValueType> =
    T extends 'null' ? null :
    T extends 'boolean' ? boolean :
    T extends 'number' ? number :
    Extract<Value, {type: T}>
;

function typeOfValue(value: Value): ValueType {
    if (value === null) {
        return 'null';
    } else if (typeof value !== 'object') {
        return typeof value as ValueType;
    } else {
        return value.type;
    }
}

function typeCheck<T extends ValueType>(value: Value, type: T): value is ValueOfType<T> {
    return typeOfValue(value) === type;
}

function valueIsScalar(value: Value): value is Scalar {
    return typeof value === 'number' || (value !== null && typeof value === 'object' && (value.type === 'generation' || value.type === 'coord'));
}

function valueIsTruthy(value: Value): boolean {
    if (value === null) {
        return false;
    } else if (typeof value === 'boolean') {
        return value;
    } else if (typeof value === 'number') {
        return value !== 0;
    } else if (value.type === 'generation') {
        return value.value !== 0;
    } else if (value.type === 'range') {
        return true;
    } else if (value.type === 'coord') {
        return true;
    } else if (value.type === 'state_specifier') {
        return true;
    } else if (value.type === 'full_state_specifier') {
        return true;
    } else {
        throw new Error(`This error should not occur (invalid value: ${value satisfies never})`);
    }
}

function valuesAreEqual(x: Value, y: Value): boolean {
    if (x === null || typeof x !== 'object' || y === null || typeof y !== 'object') {
        return x === y;
    }
    if (x.type !== y.type) {
        return false;
    }
    if (x.type === 'generation' && y.type === 'generation') {
        return x.isAbsolute === y.isAbsolute && x.value === y.value;
    } else if (x.type === 'range' && y.type === 'range') {
        return x.isInclusive === y.isInclusive && valuesAreEqual(x.start, y.start) && valuesAreEqual(x.end, y.end);
    } else if (x.type === 'coord' && y.type === 'coord') {
        return x.t === y.t && x.x === y.x && x.y === y.y;
    } else if (x.type === 'state_specifier' && y.type === 'state_specifier') {
        return stateSpecifiersAreEqual(x, y);
    } else if (x.type === 'full_state_specifier' && y.type === 'full_state_specifier') {
        // placeholder
        return x === y;
    } else {
        throw new Error(`This error should not occur (invalid value)`);
    }
}


const KEYWORDS = new Set([
    // literals
    'null', 'true', 'false', 'nop', 'off', 'on', 'unknown', 'var', 'all', 'height', 'width', 'gens',
    // operators
    'state', 'noclause', 'yesclause', 'period', 'grid',
    // statements
    'gens', 'let', 'mut', 'if', 'else', 'for', 'while',
] as const);
type Keyword = typeof KEYWORDS extends Set<infer T> ? T : never;

const SYMBOLS = new Set([
    // misc
    ';', ',', '(', ')', ':', '{', '}',
    // unary operators
    '+', '-', '~', '!', '%',
    // arithmetic binary operators
    '+', '-', '*', '/', '%', '<<', '>>', '&', '^', '|', '..', '..<',
    // boolean binary operators
    '<', '<=', '>', '>=', '==', '!=', '&&', '||', '^^', '??', '?', ':',
    // assignment operators
    '=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=', '??=',
] as const);
type XLSScriptSymbol = typeof SYMBOLS extends Set<infer T> ? T : never;

const SPECIAL_IDENTIFIERS = new Set([
    // grid parts
    'width', 'height', 'gens',
] as const);
type SpecialIdentifier = typeof SPECIAL_IDENTIFIERS extends Set<infer T> ? T : never;


const EOF = Symbol();
type EOF = typeof EOF;

const WHITESPACE = new Set([' ', '\n', '\t', '\v', '\f']);

type NewlineToken = ObjWithPos & {type: 'newline'};
type KeywordToken<T extends Keyword = Keyword> = ObjWithPos & {type: 'keyword', value: T};
type IdentifierToken = ObjWithPos & {type: 'identifier', value: string};
type NumberLiteralToken = ObjWithPos & {type: 'number-literal', value: number, raw: string};
type SymbolToken<T extends XLSScriptSymbol = XLSScriptSymbol> = ObjWithPos & {type: 'symbol', value: T};
type RLEToken = ObjWithPos & {type: 'rle', value: string};
type Token = NewlineToken | KeywordToken | IdentifierToken | NumberLiteralToken | SymbolToken | RLEToken;

const STRING_TOKEN_NAMES: {[K in Token['type']]: string} = {
    'newline': 'newline',
    'keyword': 'keyword',
    'identifier': 'identifier',
    'number-literal': 'number literal',
    'symbol': 'symbol',
    'rle': 'RLE',
};

function getTokenRaw(token: Token): string {
    if (token.type === 'newline') {
        return '\\n';
    } else if (token.type === 'number-literal') {
        return token.raw;
    } else {
        return JSON.stringify(token.value).slice(1, -1).replaceAll('\\"', '"');
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
    | XLSScriptSymbol
    | `identifier ${string}`
    | Iterable<Matcher>
;

const LINE_END: ['newline', ';', EOF] = ['newline', ';', EOF];

type MatcherReturnType<T extends Matcher> =
    T extends EOF ? EOF :
    T extends 'any' ? Token :
    T extends Token['type'] ? Extract<Token, {type: T}> :
    T extends Keyword ? KeywordToken :
    T extends XLSScriptSymbol ? SymbolToken<T> :
    T extends `identifier ${string}` ? IdentifierToken :
    // T extends (infer U)[] ? MatcherReturnType<U> :
    T extends any[] ? Token :
    never
;

function matcherToString(matcher: Matcher): string {
    if (matcher === EOF) {
        return `end of file`;
    } else if (typeof matcher === 'string') {
        if (KEYWORDS.has(matcher as Keyword)) {
            return `keyword '${matcher}'`;
        } else if (SYMBOLS.has(matcher as XLSScriptSymbol)) {
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
type SpecialIdentifierExpression = ObjWithPos & {type: 'special-identifier-expression', name: SpecialIdentifier};
type UnaryOperator =
    | '+' | '-' | '~' | '!' | '%'
    | 'state' | 'noclause' | 'yesclause' | 'period' | 'grid'
;
type CoordLiteralExpression = ObjWithPos & {type: 'coord-literal-expression', t: Expression, x: Expression, y: Expression};
type UnaryExpression = ObjWithPos & {type: 'unary-expression', op: UnaryOperator, value: Expression};
type BinaryOperator =
    |'+' | '-' | '*' | '/' | '%' | '<<' | '>>' | '&' | '^' | '|' | '..' | '..<'
    | '<' | '<=' | '>' | '>=' | '==' | '!=' | '&&' | '||' | '^^' | '??'
    | '='
;
type BinaryExpression = ObjWithPos & {type: 'binary-expression', op: BinaryOperator, left: Expression, right: Expression};
type FullStateSpecifierExpressionPart = ObjWithPos & {type: 'full-state-specifier-expression-part', gens: Expression, value: Expression};
type FullStateSpecifierExpression = ObjWithPos & {type: 'full-state-specifier-expression', parts: FullStateSpecifierExpressionPart[]};
type ConditionalExpression = ObjWithPos & {type: 'conditional-expression', condition: Expression, ifTrue: Expression, ifFalse: Expression};
type Expression = IdentifierExpression | SimpleLiteralExpression | SpecialIdentifierExpression | CoordLiteralExpression | UnaryExpression | BinaryExpression | FullStateSpecifierExpression | ConditionalExpression;
type AnyExpressionNode = Expression | FullStateSpecifierExpressionPart;

type ExpressionStatement = ObjWithPos & {type: 'expression-statement', expr: Expression};
type BlockStatement = ObjWithPos & {type: 'block-statement', data: Statement[]};
type RLEStatement = ObjWithPos & {type: 'rle-statement', rle: RLEToken, gens?: Expression, offset?: {x: Expression, y: Expression}};
type VariableDeclarationStatement = ObjWithPos & {type: 'variable-declaration-statement', mut: boolean, name: string, value?: Expression};
type IfStatement = ObjWithPos & {type: 'if-statement', condition: Expression, ifTrue: BlockStatement, ifFalse?: BlockStatement};
type ForStatement = ObjWithPos & {type: 'for-statement', name: IdentifierToken, iterateOver: Expression, value: BlockStatement};
type WhileStatement = ObjWithPos & {type: 'while-statement', condition: Expression, value: BlockStatement};
type Statement = ExpressionStatement | BlockStatement | RLEStatement | VariableDeclarationStatement | IfStatement | ForStatement | WhileStatement;
type AnyStatementNode = Statement;

type Node = AnyExpressionNode | AnyStatementNode;
type NodeType = Node['type'];

function createNode<T extends NodeType>(pos: HasPos, type: T, data: Omit<Extract<Node, {type: T}>, 'pos' | 'type'>): Extract<NodeType, {type: T}> {
    return {pos, type, ...data} as Extract<NodeType, {type: T}>;
}


class XLSScriptParser {

    currentFilePath: string;
    code: string;
    tokens: Token[];
    pos: number;

    constructor(filePath: string, code: string) {
        this.currentFilePath = filePath;
        this.code = code;
        this.tokens = this.tokenize(code);
        this.pos = 0;
    }

    getCurrentPosition(): Position {
        if (this.tokens[this.pos] === undefined) {
            return this.tokens[this.tokens.length - 1].pos;
        } else {
            return this.tokens[this.pos].pos;
        }
    }

    error(pos: HasPos | undefined, msg: string): never {
        if (pos === undefined) {
            pos = this.getCurrentPosition();
        } else if ('pos' in pos) {
            pos = pos.pos;
        }
        let trace: string[] = [];
        let text = `Error: ${msg}\n    at ${positionToString(pos)}`;
        throw new XLSScriptError(text);
    }

    readonly IDENTIFIER_REGEX = /^\p{XID_Start}\p{XID_Continue}*$/u;
    readonly NUMBER_REGEX = /^-?(0|[1-9]\d*(.[0-9]+)?(e[+-]?[1-9]\d+(.[0-9]+)?)?|0b[01]+|0o[0-7]+|0x[0-9a-f])$/iu;

    tryToConvertToSingleToken(pos: Position, value: string): Token | false | undefined {
        pos = structuredClone(pos);
        if (WHITESPACE.has(value)) {
            // undefined means whitespace
            return undefined;
        } else if (value.match(this.IDENTIFIER_REGEX)) {
            return {pos, type: 'identifier', value};
        } else if (value.match(this.NUMBER_REGEX)) {
            return {pos, type: 'number-literal', value: Number(value), raw: value};
        } else if (SYMBOLS.has(value as XLSScriptSymbol)) {
            return {pos, type: 'symbol', value: value as XLSScriptSymbol};
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
                out.push({pos, type: 'rle', value: rle});
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
                    this.error(pos, `Cannot parse input into token`);
                }
            }
            for (let token of currentLine) {
                out.push(token);
            }
            out.push({pos: {file, line: lineNumber, col}, type: 'newline'});
        }
        if (multiLineComment) {
            this.error(multiLineComment, `Unterminated multi-line comment`);
        }
        if (out.length === 0) {
            this.error({file, line: 1, col: 1}, `Empty file`);
        }
        return out;
    }

    peek<T extends Exclude<Matcher, EOF> = Exclude<Matcher, EOF>>(): MatcherReturnType<T> {
        let out = this.tokens[this.pos];
        if (out === undefined) {
            this.error(undefined, `Unexpected end of input`);
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
            this.error(undefined, `Unexpected end of input`);
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
            } else if (SYMBOLS.has(matcher as XLSScriptSymbol)) {
                return token.type === 'symbol' && token.value === matcher;
            } else if (matcher.startsWith('identifier ')) {
                return token.type === 'identifier' && token.value === matcher.slice('identifier '.length);
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
                this.error(undefined, `Expected ${matcherToString(matcher)}, got ${tokenToString(token)}`);;
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

    nextVarValueNumber: number = 0;

    primaryExpression(): Expression {
        if (this.match('identifier')) {
            // identifier lvalue
            let token = this.advance<'identifier'>();
            return createNode(token, 'identifier-expression', {name: token.value});
        } else if (this.match('null')) {
            // null literal
            let token = this.advance();
            return createNode(token, 'simple-literal-expression', {value: null});
        } else if (this.match('false')) {
            // false literal
            let token = this.advance();
            return createNode(token, 'simple-literal-expression', {value: false});
        } else if (this.match('true')) {
            // true literal
            let token = this.advance();
            return createNode(token, 'simple-literal-expression', {value: true});
        } else if (this.match('number-literal')) {
            // number literal
            let token = this.advance<'number-literal'>();
            return createNode(token, 'simple-literal-expression', {value: token.value});
        } else if (this.match('nop')) {
            // 'nop' state specifier literal
            let token = this.advance();
            let value: StateSpecifier = {type: 'state_specifier', kind: 'nop'};
            return createNode(token, 'simple-literal-expression', {value});
        } else if (this.match(['off', 'on', 'unknown'])) {
            // 'off', 'on', or 'unknown' state specifier literal
            let token = this.advance() as KeywordToken<'off' | 'on' | 'unknown'>;
            let value: StateSpecifier = {type: 'state_specifier', kind: 'cell', cell: token.value};
            return createNode(token, 'simple-literal-expression', {value});
        } else if (this.match('var')) {
            // 'var' state specifier literal
            let token = this.advance();
            let number = this.nextVarValueNumber++;
            let value: StateSpecifier = {type: 'state_specifier', kind: 'var', number};
            return createNode(token, 'simple-literal-expression', {value});
        } else if (this.match('all')) {
            // 'all' generation range literal
            let token = this.advance();
            let start: Generation = {type: 'generation', isAbsolute: true, value: 0};
            let end: Generation = {type: 'generation', isAbsolute: true, value: -1};
            let value: Range<Generation> = {type: 'range', isInclusive: true, start, end};
            return createNode(token, 'simple-literal-expression', {value});
        } else if (this.match(SPECIAL_IDENTIFIERS)) {
            // special identifier literal
            let token = this.advance() as KeywordToken<SpecialIdentifier>;
            return createNode(token, 'special-identifier-expression', {name: token.value});
        } else if (this.match('(')) {
            let token = this.advance();
            // parenthesized expression
            let value = this.expression();
            if (!this.match(',')) {
                this.eat(')');
                return value;   
            }
            // coord literal
            let t = value;
            this.eat(',');
            let x = this.expression();
            this.eat(',');
            let y = this.expression();
            this.eat(')');
            return createNode(token, 'coord-literal-expression', {t, x, y});
        } else {
            this.error(undefined, `Expected primary expression`);
        }
    }

    readonly UNARY_OPERATORS: Matcher[] = ['+', '-', '~', '!', '%', 'state', 'noclause', 'yesclause', 'period', 'grid'];

    unaryExpression(): Expression {
        if (!this.match(this.UNARY_OPERATORS)) {
            return this.primaryExpression();
        }
        let op = this.eat(this.UNARY_OPERATORS) as SymbolToken | KeywordToken;
        let value = this.primaryExpression();
        return createNode(op, 'unary-expression', {op: op.value as UnaryOperator, value});
    }

    rangeExpression(): Expression {
        let left = this.unaryExpression();
        while (this.match(['..', '..<'])) {
            let op = this.advance<'..' | '..<'>();
            let right = this.unaryExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    multiplicativeExpression(): Expression {
        let left = this.rangeExpression();
        while (this.match(['*', '/', '%'])) {
            let op = this.advance<'*' | '/' | '%'>();
            let right = this.rangeExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    additiveExpression(): Expression {
        let left = this.multiplicativeExpression();
        while (this.match(['+', '-'])) {
            let op = this.advance<'+' | '-'>();
            let right = this.multiplicativeExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    shiftExpression(): Expression {
        let left = this.additiveExpression();
        while (this.match(['<<', '>>'])) {
            let op = this.advance<'<<' | '>>'>();
            let right = this.additiveExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    relationalExpression(): Expression {
        let left = this.shiftExpression();
        while (this.match(['<', '<=', '>', '>='])) {
            let op = this.advance<'<' | '<=' | '>' | '>='>();
            let right = this.shiftExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    equalityExpression(): Expression {
        let left = this.relationalExpression();
        while (this.match(['==', '!='])) {
            let op = this.advance<'==' | '!='>();
            let right = this.relationalExpression();
            left = createNode(op, 'binary-expression', {op: op.value, left, right});
        }
        return left;
    }

    bitwiseANDExpression(): Expression {
        let left = this.equalityExpression();
        while (this.match('&')) {
            let op = this.advance();
            let right = this.equalityExpression();
            left = createNode(op, 'binary-expression', {op: '&', left, right});
        }
        return left;
    }

    bitwiseXORExpression(): Expression {
        let left = this.bitwiseANDExpression();
        while (this.match('^')) {
            let op = this.advance();
            let right = this.bitwiseANDExpression();
            left = createNode(op, 'binary-expression', {op: '^', left, right});
        }
        return left;
    }

    bitwiseORExpression(): Expression {
        let left = this.bitwiseXORExpression();
        while (this.match('|')) {
            let op = this.advance();
            let right = this.bitwiseXORExpression();
            left = createNode(op, 'binary-expression', {op: '|', left, right});
        }
        return left;
    }

    logicalANDExpression(): Expression {
        let left = this.bitwiseORExpression();
        while (this.match('&&')) {
            let op = this.advance();
            let right = this.bitwiseORExpression();
            left = createNode(op, 'binary-expression', {op: '&&', left, right});
        }
        return left;
    }

    logicalXORExpression(): Expression {
        let left = this.logicalANDExpression();
        while (this.match('^^')) {
            let op = this.advance();
            let right = this.logicalANDExpression();
            left = createNode(op, 'binary-expression', {op: '^^', left, right});
        }
        return left;
    }

    logicalORExpression(): Expression {
        let left = this.logicalXORExpression();
        while (this.match('||')) {
            let op = this.advance();
            let right = this.logicalXORExpression();
            left = createNode(op, 'binary-expression', {op: '||', left, right});
        }
        return left;
    }

    defaultToExpression(): Expression {
        let left = this.logicalORExpression();
        while (this.match('??')) {
            let op = this.advance();
            let right = this.logicalORExpression();
            left = createNode(op, 'binary-expression', {op: '??', left, right});
        }
        return left;
    }

    fullStateSpecifierExpressionPart(): FullStateSpecifierExpressionPart {
        let gens = this.rangeExpression();
        let colon = this.eat(':');
        let value = this.defaultToExpression();
        return createNode(colon, 'full-state-specifier-expression-part', {gens, value});
    }

    fullStateSpecifierExpression(): Expression {
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
        let parts: FullStateSpecifierExpressionPart[] = [];
        while (true) {
            parts.push(this.fullStateSpecifierExpressionPart());
            if (this.match(',')) {
                this.advance();
            } else {
                break;
            }            
        }
        return createNode(pos, 'full-state-specifier-expression', {parts});
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
        return createNode(questionMark, 'conditional-expression', {condition, ifTrue, ifFalse});
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
            right = createNode(op, 'binary-expression', {op: stringOp, left, right});
        }
        return createNode(op, 'binary-expression', {op: '=', left, right});
    }

    expressionStatement(): Statement {
        let pos = this.getCurrentPosition();
        let expr = this.expression();
        this.eat(LINE_END);
        return createNode(pos, 'expression-statement', {expr});
    }

    blockStatement(): BlockStatement {
        let token = this.eat('{');
        let data: Statement[] = [];
        while (!this.match('}')) {
            data.push(this.statement());
        }
        return createNode(token, 'block-statement', {data});
    }

    rleStatement(): Statement {
        let pos = this.getCurrentPosition();
        if (this.match('rle')) {
            let token = this.advance<'rle'>();
            this.eat(LINE_END);
            return createNode(pos, 'rle-statement', {rle: token});
        }
        this.eat(['identifier gen', 'gens']);
        let gens = this.expression();
        let offset: RLEStatement['offset'] = undefined;
        if (this.match(`identifier offset`)) {
            this.advance();
            let x = this.expression();
            this.eat(',');
            let y = this.expression();
            offset = {x, y};
        }
        this.eat(':');
        this.eat('newline');
        while (this.match('newline')) {
            this.advance();
        }
        let rle = this.eat('rle');
        this.eat(LINE_END);
        return createNode(pos, 'rle-statement', {rle, gens, offset});
    }

    variableDeclarationStatement(): Statement {
        let token = this.eat('let');
        let mut = false;
        if (this.match('mut')) {
            this.advance();
            mut = true;
        }
        let name = this.eat('identifier').value;
        let value: Expression | undefined = undefined;
        if (this.match('=')) {
            this.advance();
            value = this.expression();
        }
        if (!mut && value === undefined) {
            this.error(undefined, `Non-mutable variable declarations must include an initializer`);
        }
        this.eat(LINE_END);
        return createNode(token, 'variable-declaration-statement', {mut, name, value});
    }

    ifStatement(): Statement {
        let token = this.eat('if');
        this.eat('(');
        let condition = this.expression();
        this.eat(')');
        let ifTrue = this.blockStatement();
        let ifFalse: BlockStatement | undefined = undefined;
        if (this.match('else')) {
            this.advance();
            ifFalse = this.blockStatement();
        }
        return createNode(token, 'if-statement', {condition, ifTrue, ifFalse});
    }

    forStatement(): Statement {
        let token = this.eat('for');
        let name = this.eat('identifier');
        this.eat('identifier in');
        let iterateOver = this.expression();
        let value = this.blockStatement();
        return createNode(token, 'for-statement', {name, iterateOver, value});
    }

    whileStatement(): Statement {
        let token = this.eat('while');
        this.eat('(');
        let condition = this.expression();
        this.eat(')');
        let value = this.blockStatement();
        return createNode(token, 'while-statement', {condition, value});
    }

    statement(): Statement {
        while (this.match(LINE_END)) {
            this.advance();
        }
        let out: Statement;
        if (this.match('{')) {
            out = this.blockStatement();
        } else if (this.match('rle')) {
            out = this.rleStatement();
        } else if (this.match(['identifier gen', 'gens'])) {
            // advanced lookahead to see if it is a RLE statement
            // this works because RLE statements must end in a colon followed
            // by one or more newlines and then a RLE token, and this is
            // always an invalid expression, so the grammar is unambiguous
            let startPos = this.pos;
            let isRLEStatement = false;
            while (!this.match(LINE_END)) {
                if (this.match(':')) {
                    this.advance();
                    while (this.match(LINE_END)) {
                        this.advance();
                    }
                    if (this.match('rle')) {
                        isRLEStatement = true;
                    }
                    break;
                }
            }
            this.pos = startPos;
            if (isRLEStatement) {
                out = this.rleStatement();
            } else {
                out = this.expressionStatement();
            }
        } else if (this.match('let')) {
            out = this.variableDeclarationStatement();
        } else if (this.match('if')) {
            out = this.ifStatement();
        } else if (this.match('for')) {
            out = this.forStatement();
        } else if (this.match('while')) {
            out = this.whileStatement();
        } else {
            out = this.expressionStatement();
        }
        while (this.match(LINE_END)) {
            this.advance();
        }
        return out;
    }

    async program(): Promise<Statement[]> {
        let out: Statement[] = [];
        while (this.pos < this.tokens.length) {
            out.push(this.statement());
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
    states: Map<number, FullStateSpecifier>;

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
            this.runner.error(pos, `Variable ${name} is not defined`);
        }
    }

    declareVar(name: string, value: Variable): void {
        this.vars.set(name, value);
    }

    setVar(pos: HasPos, name: string, value: Value): void {
        let data = this.vars.get(name);
        if (data !== undefined) {
            if (!data.mut) {
                this.runner.error(pos, `Variable ${name} is not mutable`);
            }
            this.vars.set(name, {mut: data.mut, value});
        } else if (this.parent) {
            this.parent.setVar(pos, name, value);
        } else {
            this.runner.error(pos, `Variable ${name} is not defined`);
        }
    }

    getState(pos: HasPos, state: number): FullStateSpecifier {
        let value = this.states.get(state);
        if (value !== undefined) {
            return value;
        } else if (this.parent) {
            return this.parent.getState(pos, state);
        } else {
            this.runner.error(pos, `State ${state} is not defined`);
        }
    }

    setState(state: number, value: FullStateSpecifier): void {
        this.states.set(state, value);
    }

}


type IdentifierLvalue = {type: 'lvalue', kind: 'identifier', name: string};
type SpecialIdentifierLvalue = {type: 'lvalue', kind: 'special-identifier', name: SpecialIdentifier};
type StateLvalue = {type: 'lvalue', kind: 'state', value: number};
type GridLvalue = {type: 'lvalue', kind: 'grid', t: number, x: number, y: number};

type Lvalue = IdentifierLvalue | SpecialIdentifierLvalue | StateLvalue | GridLvalue;

type ExpressionValue = Value | Lvalue;

function isLvalue(value: ExpressionValue): value is Lvalue {
    return value !== null && typeof value === 'object' && value.type === 'lvalue';
}


class XLSScriptRunner {

    code: string;
    ulp: ULP;
    noClause: Grid;
    scope: Scope;
    callStack: Position[];

    constructor(code: string) {
        this.code = code;
        this.ulp = new ULP(2);
        this.noClause = new Grid();
        this.scope = new Scope(this, undefined);
        this.callStack = [];
    }

    error(pos: HasPos, msg: string): never {
        if ('pos' in pos) {
            pos = pos.pos;
        }
        let text = `Error: ${msg}\n    at ${positionToString(pos)}`;
        if (this.callStack) {
            for (let value of this.callStack) {
                text += `\n    at ${positionToString(value)}`;
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

    assertType<T extends ValueType>(pos: HasPos, value: Value, type: T, whatFor: string): ValueOfType<T> {
        let valueType = typeOfValue(value);
        if (valueType !== type) {
            this.error(pos, `Expected value of type '${type}' for ${whatFor}, got type '${valueType}'`);
        }
        return value as ValueOfType<T>;
    }

    castToGeneration(pos: HasPos, value: Value): Generation {
        if (typeCheck(value, 'generation')) {
            return value;
        } else if (typeCheck(value, 'number')) {
            return {type: 'generation', isAbsolute: true, value};
        } else {
            this.error(pos, `Cannot cast value of type '${typeOfValue(value)}' to generation`);
        }
    }

    resolveGeneration(pos: HasPos, value: number | Generation, relativeTo: number) {
        let grid = this.ulp.grid;
        let out: number;
        if (typeof value === 'number') {
            out = value;
        } else {
            out = value.value;
            if (!value.isAbsolute) {
                out += relativeTo;
            }
        }
        if (out < 0) {
            out += grid.gens;
        }
        if (out < 0 || out >= grid.gens) {
            this.error(pos, `Gneration out of bounds: '${out}' (expected -${grid.gens} to ${grid.gens - 1})`);
        }
        return out;
    }

    resolveGenerations(pos: HasPos, value: Value, relativeTo: number): number[] {
        if (typeCheck(value, 'number')) {
            return [value];
        } else if (typeCheck(value, 'generation')) {
            return [this.resolveGeneration(pos, value, relativeTo)];
        } else if (typeCheck(value, 'range')) {
            if (typeCheck(value.start, 'coord')) {
                this.error(pos, `Invalid type for start value of generation range: '${value.start}'`);
            }
            if (typeCheck(value.end, 'coord')) {
                this.error(pos, `Invalid type for end value of generation range: '${value.end}'`);
            }
            let start = this.resolveGeneration(pos, value.start, relativeTo);
            let end = this.resolveGeneration(pos, value.end, relativeTo);
            if (value.isInclusive) {
                end++;
            }
            let out: number[] = [];
            for (let i = start; i < end; i++) {
                out.push(i);
            }
            return out;
        } else {
            this.error(pos, `Expected value of type 'generation' or 'range<generation>', got type '${typeOfValue(value)}'`);
        }
    }

    varValueNumberMapping: {[key: number]: number} = {};

    setCells(gens: number[], x: number, y: number, value: StateSpecifier, baseT: number): void {
        let noClause = this.noClause;
        for (let t of gens) {
            if (value.noClause !== undefined) {
                noClause.set(t, x, y, value.noClause ? 1 : 0);
            }
        }
        if (value.kind === 'periodic') {
            // special case for periodic
            let cells: Cell[] = [];
            for (let i = 0; i < value.period; i++) {
                cells.push(this.ulp.getNewVar());
            }
            for (let t of gens) {
                let cell = cells[(t + baseT) % cells.length];
                this.ulp.reassign(this.ulp.grid.get(t, x, y), cell);
            }
            return;
        }
        for (let t of gens) {
            if (value.kind === 'nop') {
                // do nothing
            } else if (value.kind === 'cell' || value.kind === 'var' || value.kind === 'raw') {
                let cell: Cell;
                if (value.kind === 'var') {
                    let number = value.number;
                    if (number in this.varValueNumberMapping) {
                        cell = this.varValueNumberMapping[number];
                    } else {
                        cell = this.ulp.getNewVar();
                        this.varValueNumberMapping[number] = cell;
                    }
                } else if (value.kind === 'raw') {
                    cell = value.value;
                } else if (value.cell === 'off') {
                    cell = OFF;
                } else if (value.cell === 'on') {
                    cell = ON;
                } else if (value.cell === 'unknown') {
                    cell = this.ulp.getNewVar();
                } else {
                    throw new Error(`This error should not occur (invalid state specifier cell: ${value.cell satisfies never})`);
                }
                this.ulp.reassign(this.ulp.grid.get(t, x, y), cell);
            } else {
                throw new Error(`This error should not occur (invalid state specifier: ${value satisfies never})`);
            }
        }
    }

    getSpecialIdentifier(name: SpecialIdentifier): Value {
        let grid = this.ulp.grid;
        if (name === 'width') {
            return grid.width;
        } else if (name === 'height') {
            return grid.height;
        } else if (name === 'gens') {
            return grid.gens;
        } else {
            throw new Error(`This error should not occur (invalid special identifier: ${name satisfies never})`);
        }
    }

    setSpecialIdentifier(opPos: HasPos, name: SpecialIdentifier, value: Value): void {
        let grid = this.ulp.grid;
        let noClause = this.noClause;
        if (name === 'width' || name === 'height' || name === 'gens') {
            value = this.assertType(opPos, value, 'number', `assignment to ${name}`);
            if (name === 'width') {
                if (grid.width === value) {
                    return;
                } else if (value < grid.width) {
                    grid.shrink({right: grid.width - value});
                    noClause.shrink({right: grid.width - value});
                } else {
                    grid.expand({right: value - grid.width}, this.ulp.getNewVar.bind(this.ulp));
                    noClause.expand({right: value - grid.width}, 0);
                }
            } else if (name === 'height') {
                if (grid.height === value) {
                    return;
                } else if (value < grid.height) {
                    grid.shrink({bottom: grid.height - value});
                    noClause.shrink({right: grid.width - value});
                } else {
                    grid.expand({bottom: value - grid.height}, this.ulp.getNewVar.bind(this.ulp));
                    noClause.expand({right: value - grid.width}, 0);
                }
            } else {

            }
        } else {
            throw new Error(`This error should not occur (invalid special identifier: ${name satisfies never})`);
        }
    }

    decayLvalues(pos: HasPos, value: ExpressionValue): Value {
        if (isLvalue(value)) {
            if (value.kind === 'identifier') {
                return this.scope.getVar(pos, value.name).value;
            } else if (value.kind === 'state') {
                return this.scope.getState(pos, value.value);
            } else if (value.kind === 'special-identifier') {
                return this.getSpecialIdentifier(value.name);
            } else if (value.kind === 'grid') {
                let {t, x, y} = value;
                if (!this.ulp.grid.isInBounds(t, x, y)) {
                    this.error(pos, `Coord is out of bounds: (${t}, ${x}, ${y})`);
                } else {
                    return this.ulp.grid.get(t, x, y);
                }
            } else {
                throw new Error(`This error should not occur (invalid lvalue: ${value satisfies never})`);
            }
        } else {
            return value;
        }
    }

    coordLiteralExpression(node: CoordLiteralExpression): ExpressionValue {
        let t = this.expression(node.t);
        let startT: Generation;
        let endT: Generation;
        if (typeCheck(t, 'number')) {
            t = {type: 'generation', isAbsolute: true, value: t};
            startT = t;
            endT = t;
        } else if (typeCheck(t, 'generation')) {
            startT = t;
            endT = t;
        } else if (typeCheck(t, 'range')) {
            if (typeCheck(t.start, 'number')) {
                startT = {type: 'generation', isAbsolute: true, value: t.start};
            } else if (typeCheck(t.start, 'generation')) {
                startT = t.start;
            } else {
                this.error(node.t, `Invalid type for start of generation value range in coord literal expression: '${typeOfValue(t.start)}'`);
            }
            if (typeCheck(t.end, 'number')) {
                endT = {type: 'generation', isAbsolute: true, value: t.end};
            } else if (typeCheck(t.end, 'generation')) {
                endT = t.end;
            } else {
                this.error(node.t, `Invalid type for end of generation value range in coord literal expression: '${typeOfValue(t.end)}'`);
            }
            if (t.isInclusive) {
                endT.value += 1;
            }
        } else {
            this.error(node.t, `Invalid type for generation value in coord literal expression: '${typeOfValue(t)}'`);
        }
        let x = this.expression(node.x);
        let startX: number;
        let endX: number;
        if (typeCheck(x, 'number')) {
            startX = x;
            endX = x;
        } else if (typeCheck(x, 'range')) {
            startX = this.assertType(node.x, x.start, 'number', `start value of x range of coord literal expression`);
            endX = this.assertType(node.x, x.end, 'number', `end value of x range of coord literal expression`);
            if (x.isInclusive) {
                endX += 1;
            }
        } else {
            this.error(node.x, `Invalid type for x value of coord literal expression, expected 'number' or 'range<number>': '${typeOfValue(x)}'`);
        }
        let y = this.expression(node.y);
        let startY: number;
        let endY: number;
        if (typeCheck(y, 'number')) {
            startY = y;
            endY = y;
        } else if (typeCheck(y, 'range')) {
            startY = this.assertType(node.y, y.start, 'number', `start value of x range of coord literal expression`);
            endY = this.assertType(node.y, y.end, 'number', `end value of x range of coord literal expression`);
            if (y.isInclusive) {
                endY += 1;
            }
        } else {
            this.error(node.y, `Invalid type for x value of coord literal expression, expected 'number' or 'range<number>': '${typeOfValue(y)}'`);
        }
        if (startT === endT && startX === endX && startY === endY) {
            return {type: 'coord', t: startT, x: endX, y: endY};
        } else {
            let start: Coord = {type: 'coord', t: startT, x: startX, y: startY};
            let end: Coord = {type: 'coord', t: endT, x: endX, y: endY};
            return {type: 'range', isInclusive: false, start, end};
        }
    }

    unaryExpression(node: UnaryExpression): ExpressionValue {
        let op = node.op;
        let arg = this.expression(node.value);
        let argPos = node.value.pos;
        if (op === '+') {
            return this.assertType(argPos, arg, 'number', `unary + operator`);
        } else if (op === '-') {
            return -this.assertType(argPos, arg, 'number', `unary - operator`);
        } else if (op === '~') {
            return Number(~BigInt(this.assertType(argPos, arg, 'number', `unary ~ operator`)));
        } else if (op === '!') {
            return !valueIsTruthy(arg);
        } else if (op === '%') {
            let value = this.assertType(argPos, arg, 'number', `${op} operator`);
            return {type: 'generation', isAbsolute: false, value};
        } else if (op === 'state') {
            let value = this.assertType(argPos, arg, 'number', `state operator`);
            return {type: 'lvalue', kind: 'state', value};
        } else if (op === 'noclause' || op === 'yesclause') {
            let value = structuredClone(this.assertType(argPos, arg, 'state_specifier', `${op} operator`));
            value.noClause = op === 'noclause';
            return value;
        } else if (op === 'period') {
            let period = this.assertType(argPos, arg, 'number', `period operator`);
            return {type: 'state_specifier', kind: 'periodic', period};
        } else if (op === 'grid') {
            let {t: gen, x, y} = this.assertType(argPos, arg, 'coord', `grid operator`);
            let t = this.resolveGeneration(argPos, gen, 0);
            return {type: 'lvalue', kind: 'grid', t, x, y};
        } else {
            throw new Error(`This error should not occur (invalid unary operator: ${op satisfies never})`);
        }
    }

    binaryExpression(node: BinaryExpression): ExpressionValue {
        let op = node.op;
        let left = this._expression(node.left);
        let leftPos = node.left.pos;
        // first the assignment operator
        if (op === '=') {
            if (!isLvalue(left)) {
                this.error(leftPos, `Left-hand operand of assignment operator is not an lvalue`);
            }
            let right = this.expression(node.right);
            if (left.kind === 'identifier') {
                this.scope.setVar(leftPos, left.name, right);
            } else if (left.kind === 'special-identifier') {
                this.setSpecialIdentifier(node, left.name, right);
            } else if (left.kind === 'state') {
                this.scope.setState(left.value, this.assertType(node, right, 'full_state_specifier', `assigning to state`));
            } else if (left.kind === 'grid') {
                let {t, x, y} = left;
                if (!this.ulp.grid.isInBounds(t, x, y)) {
                    this.error(node, `Coord is out of bounds: (${t}, ${x}, ${y})`);
                } else {
                    right = this.assertType(node, right, 'state_specifier', `assignment to grid`);
                    this.setCells([t], x, y, right, 0);
                }
            } else {
                throw new Error(`This error should not occur (invalid lvalue: ${left satisfies never})`);
            }
            return right;
        }
        left = this.decayLvalues(node, left);
        // next the short-circuiting operators
        if (op === '&&') {
            if (valueIsTruthy(left)) {
                return valueIsTruthy(this.expression(node.right));
            } else {
                return false;
            }
        } else if (op === '||') {
            if (valueIsTruthy(left)) {
                return true;
            } else {
                return valueIsTruthy(this.expression(node.right));;
            }
        } else if (op === '??') {
            if (left === null) {
                return this.expression(node.right);
            } else {
                return left;
            }
        }
        // then the operators that don't work on only numbers
        let right = this.expression(node.right);
        let rightPos = node.right.pos;
        if (op === '..' || op === '..<') {
            let isInclusive = op === '..';
            if (!valueIsScalar(left)) {
                this.error(node, `Cannot make range out of non-scalar type ${typeOfValue(left)}`);
            }
            if (!valueIsScalar(right)) {
                this.error(node, `Cannot make range out of non-scalar type ${typeOfValue(right)}`);
            }
            if (typeOfValue(left) !== typeOfValue(right)) {
                this.error(node, `Cannot make range out of values of two different types '${typeOfValue(left)}' and '${typeOfValue(right)}'`);
            }
            if (typeCheck(left, 'number') && typeCheck(right, 'number')) {
                return {type: 'range', isInclusive, start: left, end: right};
            } else if ((typeCheck(left, 'number') || typeCheck(left, 'generation')) && (typeCheck(right, 'number') || typeCheck(right, 'generation'))) {
                let start = this.castToGeneration(node, left);
                let end = this.castToGeneration(node, right);
                return {type: 'range', isInclusive, start, end};
            } else if (typeCheck(left, 'coord') && typeCheck(right, 'coord')) {
                return {type: 'range', isInclusive, start: left, end: right};
            } else {
                this.error(node, `Invalid types for ${op} operator`);
            }
        } else if (op === '==') {
            return valuesAreEqual(left, right);
        } else if (op === '!=') {
            return !valuesAreEqual(left, right);
        } else if (op === '^^') {
            return valueIsTruthy(left) !== valueIsTruthy(right);
        }
        // finally the operators that work on numbers
        left = this.assertType(leftPos, left, 'number', `${op} operator`);
        right = this.assertType(rightPos, right, 'number', `${op} operator`);
        if (op === '+') {
            return left + right;
        } else if (op === '-') {
            return left - right;
        } else if (op === '*') {
            return left * right;
        } else if (op === '/') {
            return left / right;
        } else if (op === '%') {
            return left % right;
        } else if (op === '<<') {
            return Number(BigInt(left) << BigInt(right));
        } else if (op === '>>') {
            return Number(BigInt(left) >> BigInt(right));
        } else if (op === '&') {
            return Number(BigInt(left) & BigInt(right));
        } else if (op === '^') {
            return Number(BigInt(left) ^ BigInt(right));
        } else if (op === '|') {
            return Number(BigInt(left) | BigInt(right));
        } else if (op === '<') {
            return left < right;
        } else if (op === '<=') {
            return left <= right;
        } else if (op === '>') {
            return left > right;
        } else if (op === '>=') {
            return left >= right;
        } else {
            throw new Error(`This error should not occur (invalid binary operator: ${op satisfies never})`);
        }
    }

    fullStateSpecifierExpression(node: FullStateSpecifierExpression): Value {
        let parts: FullStateSpecifierPart[] = [];
        for (let part of node.parts) {
            let value = this.expression(part.gens);
            let gens: Generation | Range<Generation>;
            if (typeCheck(value, 'number')) {
                gens = {type: 'generation', isAbsolute: false, value};
            } else if (typeCheck(value, 'generation')) {
                gens = value;
            } else if (typeCheck(value, 'range')) {
                let isInclusive = value.isInclusive;
                let start: Generation;
                if (typeCheck(value.start, 'number')) {
                    start = {type: 'generation', isAbsolute: true, value: value.start};
                } else if (typeCheck(value.start, 'generation')) {
                    start = value.start;
                } else {
                    this.error(part.gens, `Expected value of type 'number' or 'generation' for state specifier generation label range start generation, got type '${typeOfValue(value.start)}'`);
                }
                let end: Generation;
                if (typeCheck(value.end, 'number')) {
                    end = {type: 'generation', isAbsolute: true, value: value.end};
                } else if (typeCheck(value.end, 'generation')) {
                    end = value.end;
                } else {
                    this.error(part.gens, `Expected value of type 'number' or 'generation' for state specifier generation label range end generation, got type '${typeOfValue(value.end)}'`);
                }
                gens = {type: 'range', isInclusive, start, end};
            } else {
                this.error(part.gens, `Expected value of type 'number', 'generation', or 'range', for state specifier generation label, got type '${typeOfValue(value)}'`);
            }
            let specifier = this.assertType(part.value, this.expression(part.value), 'state_specifier', `specifier part of full state specifier`);
            parts.push({gens, value: specifier});
        }
        return {type: 'full_state_specifier', parts};
    }

    _expression(node: Expression): ExpressionValue {
        if (node.type === 'identifier-expression') {
            return {type: 'lvalue', kind: 'identifier', name: node.name};
        } else if (node.type === 'simple-literal-expression') {
            return node.value;
        } else if (node.type === 'special-identifier-expression') {
            return {type: 'lvalue', kind: 'special-identifier', name: node.name};
        } else if (node.type === 'coord-literal-expression') {
            return this.coordLiteralExpression(node);
        } else if (node.type === 'unary-expression') {
            return this.unaryExpression(node);
        } else if (node.type === 'binary-expression') {
            return this.binaryExpression(node);
        } else if (node.type === 'full-state-specifier-expression') {
            return this.fullStateSpecifierExpression(node);
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
        return this.decayLvalues(node, this._expression(node));
    }

    blockStatement(node: BlockStatement): void {
        this.pushScope();
        for (let value of node.data) {
            this.statement(value);
        }
        this.popScope();
    }

    rleStatement(node: RLEStatement): void {
        // get the pattern corresponding to the RLE
        let rle = node.rle.value;
        let p: IdentityPattern;
        try {
            p = IdentityPattern.loadRLE(rle);
        } catch (error) {
            if (error instanceof LifewebError) {
                this.error(node.rle, error.message);
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
        let xOffset = 0;
        let yOffset = 0;
        if (node.offset) {
            xOffset = this.assertType(node.offset.x, this.expression(node.offset.x), 'number', `RLE X offset`);
            yOffset = this.assertType(node.offset.y, this.expression(node.offset.y), 'number', `RLE Y offset`);
        }
        let grid = this.ulp.grid;
        if (xOffset < 0) {
            grid.expand({left: -xOffset}, this.ulp.getNewVar.bind(this.ulp));
            xOffset = 0;
        }
        if (yOffset < 0) {
            grid.expand({top: -xOffset}, this.ulp.getNewVar.bind(this.ulp));
            yOffset = 0;
        }
        let maxX = width + xOffset;
        let maxY = height + yOffset;
        if (maxX > grid.width) {
            grid.expand({right: maxX - grid.width}, this.ulp.getNewVar.bind(this.ulp));
        }
        if (maxY > grid.height) {
            grid.expand({bottom: maxY - grid.height}, this.ulp.getNewVar.bind(this.ulp));
        }
        // copy the states into the pattern
        let baseGens: number[];
        if (node.gens === undefined) {
            baseGens = [0];
        } else {
            baseGens = this.resolveGenerations(node.gens, this.expression(node.gens), 0);
        }
        for (let y = 0; y < p.height; y++) {
            for (let x = 0; x < p.width; x++) {
                let state = p.get(x, y);
                let x2 = x + xOffset;
                let y2 = y + yOffset;
                for (let part of this.scope.getState(node.rle, state).parts) {
                    for (let baseGen of baseGens) {
                        let gens = this.resolveGenerations(node.gens ?? node, part.gens, baseGen);
                        this.setCells(gens, x2, y2, part.value, baseGen);
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
            value = null;
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
        let value = this.expression(node.iterateOver);
        this.pushScope();
        this.scope.declareVar(node.name.value, {mut: true, value: null});
        if (!typeCheck(value, 'range')) {
            this.error(node.iterateOver, `Expected value of type 'range' for for statement, got type '${typeOfValue(value)}`);
        }
        let start = value.start;
        let end = value.end;
        if (typeCheck(start, 'number') && typeCheck(end, 'number')) {
            end += (value.isInclusive ? 1 : 0);
            for (let i = start; i < end + (value.isInclusive ? 1 : 0); i++) {
                this.scope.setVar(node.name, node.name.value, i);
                this.blockStatement(node.value);
            }
        } else if (typeCheck(start, 'generation') && typeCheck(end, 'generation')) {
            start = this.resolveGeneration(node.iterateOver, start, 0);
            end = this.resolveGeneration(node.iterateOver, end, 0)
            for (let i = start; i < end + (value.isInclusive ? 1 : 0); i++) {
                this.scope.setVar(node.name, node.name.value, i);
                this.blockStatement(node.value);
            }
        } else if (typeCheck(start, 'coord') && typeCheck(end, 'coord')) {
            let startT = this.resolveGeneration(node.iterateOver, start.t, 0);
            let endT = this.resolveGeneration(node.iterateOver, end.t, 0);
            for (let t = startT; t < endT; t++) {
                for (let y = start.y; y < end.y + (value.isInclusive ? 1 : 0); y++) {
                    for (let x = start.x; x < end.x + (value.isInclusive ? 1 : 0); x++) {
                        let gen: Generation = {type: 'generation', isAbsolute: true, value: t};
                        let value: Coord = {type: 'coord', t: gen, x, y};
                        this.scope.setVar(node.name, node.name.value, value);
                        this.blockStatement(node.value);
                    }
                }
            }
        } else {
            throw new Error(`This error should not occur (invalid scalar)`);
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

    getRuleFunc(): ClauseFunc {
    }

    async program(data: Statement[]): Promise<ULP> {
        for (let node of data) {
            this.statement(node);
        }
        let func = this.getRuleFunc();
        let grid = this.ulp.grid;
        let bg = this.ulp.background;
        let noClause = this.noClause;
        grid.expand({top: 1, bottom: 1, left: 1, right: 1}, 0);
        for (let t = 0; t < grid.gens; t++) {
            grid.set(t, 0, 0, bg.get(0, grid.width - 1, grid.height - 1));
            grid.set(t, grid.width - 1, grid.height - 1, bg.get(0, 0, 0));
            for (let x = 1; x < grid.width - 1; x++) {
                grid.set(t, x, 0, bg.get(0, x, grid.width - 1));
                grid.set(t, x, grid.width - 1, bg.get(0, 0, x));
            }
        }
        noClause.expand({top: 1, bottom: 1, left: 1, right: 1}, 1);
        this.ulp.addClauses(func, noClause);
        return this.ulp;
    }

}


export async function runScript(code: string, filePath: string = '') {
    let parser = new XLSScriptParser(filePath, code);
    let data = await parser.program();
    let runner = new XLSScriptRunner(code);
    return await runner.program(data);
}

export async function runFile(filePath: string): Promise<ULP> {
    let fs = await import('node:fs/promises');
    let code = (await fs.readFile(filePath)).toString();
    return await runScript(code, filePath);
}
