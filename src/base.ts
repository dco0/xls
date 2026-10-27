

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


export interface CoordLike {
    t: number;
    x: number;
    y: number;
}

export class Coord implements CoordLike {

    t: number;
    x: number;
    y: number;

    constructor(t: number, x: number, y: number) {
        this.t = t;
        this.x = x;
        this.y = y;
    }

    copy(): Coord {
        return new Coord(this.t, this.x, this.y);
    }

    eq(other: Coord): boolean {
        return this.t === other.t && this.x === other.x && this.y === other.y;
    }

    add(other: Coord): Coord {
        return new Coord(this.t + other.t, this.x + other.x, this.y + other.y);
    }

    sub(other: Coord): Coord {
        return new Coord(this.t - other.t, this.x - other.x, this.y - other.y);
    }

}

export function coord(t: number, x: number, y: number): Coord {
    return new Coord(t, x, y);
}


export interface GridLike {

    width: number;
    height: number;
    gens: number;

    data: Cell[][][];

    eq(other: GridLike): boolean;

    copy(): GridLike;

    isInBounds(t: number, x: number, y: number): boolean;
    isInBounds(x: number, y: number): boolean;
    isInBounds(coord: CoordLike): boolean;

    get(t: number, x: number, y: number): Cell;
    get(coord: CoordLike): Cell;

    getAllowOOB(t: number, x: number, y: number): Cell | undefined;
    getAllowOOB(coord: CoordLike): Cell | undefined;

    set(t: number, x: number, y: number, value: Cell): void;
    set(pos: CoordLike, value: Cell): void;

    entries(): IterableIterator<[Coord, Cell]>;
    coords(): IterableIterator<Coord>;
    cells(): IterableIterator<Cell>;

    flipHorizontal(): void;
    flipVertical(): void;
    rotateLeft(): void;
    rotateRight(): void;
    rotate180(): void;
    flipDiagonal(): void;
    flipAntiDiagonal(): void;

}


export class Grid implements GridLike {

    width: number = 0;
    height: number = 0;
    gens: number = 0;

    data: Cell[][][] = [];
    
    eq(other: GridLike): boolean {
        return this === other;
    }

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
    isInBounds(coord: CoordLike): boolean;
    isInBounds(_t: number | CoordLike, _x?: number, _y?: number): boolean {
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
    get(coord: CoordLike): Cell;
    get(_t: number | CoordLike, _x?: number, _y?: number): Cell {
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
    getAllowOOB(coord: CoordLike): Cell | undefined;
    getAllowOOB(_t: number | CoordLike, _x?: number, _y?: number): Cell | undefined {
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

    set(t: number, x: number, y: number, value: Cell): void;
    set(pos: CoordLike, value: Cell): void;
    set(_t: number | CoordLike, _x: number | Cell, _y?: number, _value?: Cell): void {
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

    expand({top = 0, bottom = 0, left = 0, right = 0, start = 0, end = 0}: {top?: number, bottom?: number, left?: number, right?: number, start?: number, end?: number}, value: Cell | (() => Cell)): void {
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
    }

    shrink({top = 0, bottom = 0, left = 0, right = 0, start = 0, end = 0}: {top?: number, bottom?: number, left?: number, right?: number, start?: number, end?: number}): void {
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
    }

    flipHorizontal(): void {
        this.data = this.data.map(grid => grid.map(row => row.reverse()));
    }

    flipVertical(): void {
        this.data = this.data.map(grid => grid.reverse());
    }

    rotateLeft(): void {
        this.flipDiagonal();
        this.flipVertical();
    }

    rotateRight(): void {
        this.flipDiagonal()
        this.flipHorizontal();
    }

    rotate180(): void {
        this.flipHorizontal()
        this.flipVertical();
    }

    flipDiagonal(): void {
        this.data = this.data.map(grid => grid[0].map((_, i) => grid.map(row => row[i])));
    }

    flipAntiDiagonal(): void {
        this.flipDiagonal()
        this.rotate180();
    }

}


export class GridSlice implements GridLike {

    grid: Grid;
    start: Coord;
    end: Coord;

    width: number;
    height: number;
    gens: number;

    constructor(grid: Grid | GridSlice, start: Coord, end: Coord) {
        if (grid instanceof GridSlice) {
            start = start.add(grid.start);
            end = end.add(grid.start);
            grid = grid.grid;
        }
        this.grid = grid;
        this.start = start;
        this.end = end;
        this.width = this.end.x - this.start.x;
        this.height = this.end.y - this.start.y;
        this.gens = this.end.t - this.start.t;
    }

    get data(): Cell[][][] {
        let out: Cell[][][] = [];
        for (let t = 0; t < this.gens; t++) {
            let layer: Cell[][] = [];
            for (let y = 0; y < this.height; y++) {
                let fullRow = this.grid.data[t + this.start.t][y + this.start.y];
                layer.push(fullRow.slice(this.start.x, this.end.x));
            }
            out.push(layer);
        }
        return out;
    }

    copy(): GridSlice {
        return new GridSlice(this.grid, this.start.copy(), this.end.copy());
    }

    eq(other: GridLike): boolean {
        return other instanceof GridSlice && this.grid === other.grid && this.start.eq(other.start) && this.end.eq(other.end);
    }

    isInBounds(t: number, x: number, y: number): boolean;
    isInBounds(x: number, y: number): boolean;
    isInBounds(coord: CoordLike): boolean;
    isInBounds(_t: number | CoordLike, _x?: number, _y?: number): boolean {
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
    get(coord: CoordLike): Cell;
    get(_t: number | CoordLike, _x?: number, _y?: number): Cell {
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
        return this.grid.data[t + this.start.t][y + this.start.y][x + this.start.x];
    }

    getAllowOOB(t: number, x: number, y: number): Cell | undefined;
    getAllowOOB(coord: CoordLike): Cell | undefined;
    getAllowOOB(_t: number | CoordLike, _x?: number, _y?: number): Cell | undefined {
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
            return this.grid.data[t + this.start.t][y + this.start.y][x + this.start.x];
        }
    }

    set(t: number, x: number, y: number, value: Cell): void;
    set(pos: CoordLike, value: Cell): void;
    set(_t: number | CoordLike, _x: number | Cell, _y?: number, _value?: Cell): void {
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
        this.grid.data[t + this.start.t][y + this.start.y][x + this.start.x] = value;
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

    flipHorizontal(): void {
        for (let t = 0; t < this.gens; t++) {
            for (let y = 0; y < this.height; y++) {
                for (let x = 0; x < this.width / 2; x++) {
                    let left = this.get(t, x, y);
                    let right = this.get(t, this.width - x - 1, y);
                    this.set(t, x, y, right);
                    this.set(t, this.width - x - 1, y, left);
                }
            }
        }
    }

    flipVertical(): void {
        for (let t = 0; t < this.gens; t++) {
            for (let x = 0; x < this.width; x++) {
                for (let y = 0; y < this.height / 2; y++) {
                    let top = this.get(t, x, y);
                    let bottom = this.get(t, x, this.height - y - 1);
                    this.set(t, x, y, bottom);
                    this.set(t, x, this.height - y - 1, top);
                }
            }
        }
    }

    rotateLeft(): void {
        this.flipDiagonal();
        this.flipVertical();
    }

    rotateRight(): void {
        this.flipDiagonal()
        this.flipHorizontal();
    }

    rotate180(): void {
        this.flipHorizontal()
        this.flipVertical();
    }

    flipDiagonal(): void {
        for (let t = 0; t < this.gens; t++) {
            for (let y = 0; y < this.height; y++) {
                for (let x = 0; x < this.width; x++) {
                    if (y > x) {
                        continue;
                    }
                    let upperRight = this.get(t, x, y);
                    let lowerLeft = this.get(t, y, x);
                    this.set(t, x, y, lowerLeft);
                    this.set(t, this.width - x - 1, y, upperRight);
                }
            }
        }
    }

    flipAntiDiagonal(): void {
        this.flipDiagonal()
        this.rotate180();
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

    addClauses(func: ClauseFunc, noclause?: Grid): void {
        let grid = this.grid;
        for (let {t, x, y} of this.grid.coords()) {
            if (t === this.grid.gens - 1) {
                continue;
            }
            if (noclause && noclause.get(t, x, y) === 1) {
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
