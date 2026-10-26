

export class XLSError extends Error {

    name: string = 'XLSError';
    [Symbol.toStringTag]: string = 'XLSError';

}


export type Cell = number;

export const OFF = 0;
export const ON = 1;


export class LookupTableClauseFunc {

    readonly ulp: ULP;

    varCount: number;
    data: number[];

    constructor(ulp: ULP, varCount: number) {
        this.ulp = ulp;
        this.varCount = varCount;
        this.data = new Array(ulp.maxStates ** varCount);
        this.data.fill(0);
    }
    
    copy(ulp?: ULP): LookupTableClauseFunc {
        let out = new LookupTableClauseFunc(ulp ?? this.ulp, this.varCount);
        out.data = structuredClone(this.data);
        return out;
    }

}


export type ClauseFunc = LookupTableClauseFunc;


export class Clause {
    
    readonly ulp: ULP;

    func: ClauseFunc;
    data: Cell[];

    constructor(ulp: ULP, func: ClauseFunc, data: number[]) {
        this.ulp = ulp;
        this.func = func;
        this.data = data;
    }

    copy(ulp?: ULP, func?: LookupTableClauseFunc): Clause {
        let out = new Clause(ulp ?? this.ulp, func ?? this.func, structuredClone(this.data));
        return out;
    }

}


export class Coord {

    t: number;
    x: number;
    y: number;

    constructor(t: number, x: number, y: number) {
        this.t = t;
        this.x = x;
        this.y = y;
    }

    eq(other: Coord): boolean {
        return this.t === other.t && this.x === other.x && this.y === other.y;
    }

}

export function coord(t: number, x: number, y: number): Coord {
    return new Coord(t, x, y);
}


export class Grid {

    width: number = 0;
    height: number = 0;
    gens: number = 0;

    data: Cell[][][] = [];

    copy(): Grid {
        let out = new Grid();
        out.width = this.width;
        out.height = this.height;
        out.gens = this.gens;
        out.data = structuredClone(this.data);
        return out;
    }

    isInBounds(t: number, x: number, y: number): boolean;
    isInBounds(x: number, y: number): boolean;
    isInBounds(coord: Coord): boolean;
    isInBounds(_t: number | Coord, _x?: number, _y?: number): boolean {
        let t: number;
        let x: number;
        let y: number;
        if (typeof _t === 'object') {
            t = _t.t;
            x = _t.x;
            y = _t.y;
        } else if (_y === undefined) {
            t = 0;
            x = _t;
            y = _x as number;
        } else {
            t = _t;
            x = _x as number;
            y = _y as number;
        }
        return t >= 0 && t < this.gens && x >= 0 && x < this.width && y >= 0 && y < this.height;
    }

    get(t: number, x: number, y: number): Cell;
    get(coord: Coord): Cell;
    get(_t: number | Coord, _x?: number, _y?: number): Cell {
        let t: number;
        let x: number;
        let y: number;
        if (typeof _t === 'object') {
            t = _t.t;
            x = _t.x;
            y = _t.y;
        } else {
            t = _t;
            x = _x as number;
            y = _y as number;
        }
        if (!this.isInBounds(t, x, y)) {
            throw new Error(`Out of bounds get: t = ${t}, x = ${x}, y = ${y}`);
        }
        return this.data[t][y][x];
    }

    getAllowOOB(t: number, x: number, y: number): Cell | undefined;
    getAllowOOB(coord: Coord): Cell | undefined;
    getAllowOOB(_t: number | Coord, _x?: number, _y?: number): Cell | undefined {
        let t: number;
        let x: number;
        let y: number;
        if (typeof _t === 'object') {
            t = _t.t;
            x = _t.x;
            y = _t.y;
        } else {
            t = _t;
            x = _x as number;
            y = _y as number;
        }
        if (!this.isInBounds(t, x, y)) {
            return undefined;
        } else {
            return this.data[t][y][x];
        }
    }

    set(t: number, x: number, y: number, value: Cell): this;
    set(pos: Coord, value: Cell): this;
    set(_t: number | Coord, _x: number | Cell, _y?: number, _value?: Cell): this {
        let value: Cell;
        let t: number;
        let x: number;
        let y: number;
        if (typeof _t === 'object') {
            t = _t.t;
            x = _t.x;
            y = _t.y;
            value = _x;
        } else {
            t = _t;
            x = _x;
            y = _y as number;
            value = _value as Cell;
        }
        if (!this.isInBounds(t, x, y)) {
            throw new Error(`Out of bounds set: t = ${t}, x = ${x}, y = ${y}`);
        }
        this.data[t][y][x] = value;
        return this;
    }

    entries(): IterableIterator<[Coord, Cell]> {
        let t = 0;
        let x = 0;
        let y = 0;
        return {
            [Symbol.iterator]() {
                return this;
            },
            // arrow function so it uses the `this` context of the Grid
            next: () => {
                if (t >= this.gens) {
                    return {done: true, value: undefined};
                }
                let pos = coord(t, x, y);
                return {done: false, value: [pos, this.get(pos)]};
            }
        };
    }

    coords(): IterableIterator<Coord> {
        let t = 0;
        let x = 0;
        let y = 0;
        return {
            [Symbol.iterator]() {
                return this;
            },
            // arrow function so it uses the `this` context of the Grid
            next: () => {
                if (t >= this.gens) {
                    return {done: true, value: undefined};
                }
                let out = {done: false, value: coord(t, x, y)};
                x++;
                if (x >= this.width) {
                    x = 0;
                    y++;
                    if (y >= this.height) {
                        y = 0;
                        t++;
                    }
                }
                return out;
            }
        };
    }

    cells(): IterableIterator<Cell> {
        let t = 0;
        let x = 0;
        let y = 0;
        return {
            [Symbol.iterator]() {
                return this;
            },
            // arrow function so it uses the `this` context of the Grid
            next: () => {
                if (t >= this.gens) {
                    return {done: true, value: undefined};
                }
                let out = {done: false, value: this.get(coord(t, x, y))};
                x++;
                if (x >= this.width) {
                    x = 0;
                    y++;
                    if (y >= this.height) {
                        y = 0;
                        t++;
                    }
                }
                return out;
            }
        };
    }

    expand({top = 0, bottom = 0, left = 0, right = 0, start = 0, end = 0}: {top?: number, bottom?: number, left?: number, right?: number, start?: number, end?: number}, value: Cell | (() => Cell)): this {
        let newWidth = this.width + left + right;
        let newHeight = this.height + top + bottom;
        let newGens = this.gens + start + end;
        // first make empty placeholders
        let emptyRow: Cell[] = [];
        for (let i = 0; i < newWidth; i++) {
            emptyRow.push(typeof value === 'function' ? value() : value);
        }
        let emptyLayer: Cell[][] = [];
        for (let i = 0; i < newHeight; i++) {
            emptyLayer.push(structuredClone(emptyRow));
        }
        // then paste them in
        for (let layer of this.data) {
            for (let row of layer) {
                for (let i = 0; i < left; i++) {
                    row.unshift(typeof value === 'function' ? value() : value);
                }
                for (let i = 0; i < right; i++) {
                    row.push(typeof value === 'function' ? value() : value);
                }
            }
            for (let i = 0; i < top; i++) {
                layer.unshift(structuredClone(emptyRow));
            }
            for (let i = 0; i < bottom; i++) {
                layer.push(structuredClone(emptyRow));
            }
        }
        for (let i = 0; i < start; i++) {
            this.data.unshift(structuredClone(emptyLayer));
        }
        for (let i = 0; i < end; i++) {
            this.data.push(structuredClone(emptyLayer));
        }
        this.width = newWidth;
        this.height = newHeight;
        this.gens = newGens;
        return this;
    }

    shrink({top = 0, bottom = 0, left = 0, right = 0, start = 0, end = 0}: {top?: number, bottom?: number, left?: number, right?: number, start?: number, end?: number}): this {
        let newWidth = this.width - left - right;
        let newHeight = this.height - top - bottom;
        let newGens = this.gens - start - end;
        for (let layer of this.data) {
            for (let row of layer) {
                for (let i = 0; i < left; i++) {
                    row.shift();
                }
                for (let i = 0; i < right; i++) {
                    row.pop();
                }
            }
            for (let i = 0; i < top; i++) {
                layer.shift();
            }
            for (let i = 0; i < bottom; i++) {
                layer.pop();
            }
        }
        for (let i = 0; i < start; i++) {
            this.data.shift();
        }
        for (let i = 0; i < end; i++) {
            this.data.pop();
        }
        this.height = newHeight;
        this.width = newWidth;
        this.gens = newGens;
        return this;
    }

    flipHorizontal(): this {
        this.data = this.data.map(grid => grid.map(row => row.reverse()));
        return this;
    }

    flipVertical(): this {
        this.data = this.data.map(grid => grid.reverse());
        return this;
    }

    rotateLeft(): this {
        return this.flipDiagonal().flipVertical();
    }

    rotateRight(): this {
        return this.flipDiagonal().flipHorizontal();
    }

    rotate180(): this {
        return this.flipHorizontal().flipVertical();
    }

    flipDiagonal(): this {
        this.data = this.data.map(grid => grid[0].map((_, i) => grid.map(row => row[i])));
        return this;
    }

    flipAntiDiagonal(): this {
        return this.flipDiagonal().rotate180();
    }

}


export class Options {

    periodic: {dx: number, dy: number, period: number} | undefined = undefined;

    copy(): Options {
        let out = new Options();
        out.periodic = structuredClone(this.periodic);
        return out;
    }

}


export class ULP {

    readonly maxStates: number;

    varCount: number = 0;

    funcs: ClauseFunc[] = [];
    clauses: Clause[] = [];
    
    grid: Grid;
    background: Grid;

    options: Options;

    constructor(maxStates: number) {
        this.maxStates = maxStates;
        this.grid = new Grid();
        this.background = new Grid();
        this.options = new Options();
    }

    copy(): ULP {
        let out = new ULP(this.maxStates);
        let funcMap = new Map<ClauseFunc, ClauseFunc>();
        for (let func of this.funcs) {
            let copied = func.copy(out);
            out.funcs.push(copied);
            funcMap.set(func, copied);
        }
        for (let clause of this.clauses) {
            let newFunc = funcMap.get(clause.func);
            if (newFunc === undefined) {
                throw new Error(`Clause function cannot be mapped`);
            }
            let copied = clause.copy(out, newFunc);
            out.clauses.push(copied);
        }
        out.grid = this.grid.copy();
        out.background = this.background.copy();
        out.options = this.options.copy();
        return out;
    }

    getNewVar(): number {
        return this.maxStates + (this.varCount++);
    }

    addLookupTableClauseFunc(varCount: number): LookupTableClauseFunc {
        let out = new LookupTableClauseFunc(this, varCount);
        this.funcs.push(out);
        return out;
    }

    addClause(func: ClauseFunc, data: number[]): Clause {
        let out = new Clause(this, func, data);
        this.clauses.push(out);
        return out;
    }

    reassign(from: Cell, to: Cell): void {
        for (let clause of this.clauses) {
            for (let i = 0; i < clause.data.length; i++) {
                if (clause.data[i] === from) {
                    clause.data[i] = to;
                }
            }
        }
        for (let [coord, cell] of this.grid.entries()) {
            if (cell === from) {
                this.grid.set(coord, to);
            }
        }
    }

    setVar(cell: Cell, value: Cell): void {
        this.reassign(cell, value);
    }

    bindVars(x: Cell, y: Cell): void {
        this.reassign(x, y);
    }

    addClauses(func: ClauseFunc, noClause?: Grid): void {
        let grid = this.grid;
        for (let {t, x, y} of this.grid.coords()) {
            if (t === this.grid.gens - 1) {
                continue;
            }
            if (noClause && noClause.get(t, x, y) === 1) {
                continue;
            }
            this.addClause(func, [
                grid.get(t + 1, x, y),
                grid.getAllowOOB(t, x - 1, y - 1) ?? 0,
                grid.getAllowOOB(t, x, y - 1) ?? 0,
                grid.getAllowOOB(t, x + 1, y - 1) ?? 0,
                grid.getAllowOOB(t, x - 1, y) ?? 0,
                grid.get(t, x, y) ?? 0,
                grid.getAllowOOB(t, x + 1, y) ?? 0,
                grid.getAllowOOB(t, x - 1, y + 1) ?? 0,
                grid.getAllowOOB(t, x, y + 1) ?? 0,
                grid.getAllowOOB(t, x + 1, y + 1) ?? 0,
            ]);
        }
    }

    removeUnusedVars(): void {
        this.varCount = 0;
        let mapping: {[key: number]: number} = {};
        for (let i = 0; i < this.maxStates; i++) {
            mapping[i] = i;
        }
        for (let clause of this.clauses) {
            for (let i = 0; i < clause.data.length; i++) {
                let cell = clause.data[i];
                if (cell in mapping) {
                    clause.data[i] = mapping[cell];
                } else {
                    let value = this.getNewVar();
                    clause.data[i] = value;
                    mapping[cell] = value;
                }
            }
        }
        for (let [coord, cell] of this.grid.entries()) {
            this.grid.set(coord, mapping[cell]);
        }
    }

    normalize(): void {
        this.removeUnusedVars();
    }

}
