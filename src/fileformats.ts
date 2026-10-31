
import {XLSError, ULP, LookupTableClauseFunc, ClauseFunc, Options, GridCell} from "./base.js";

export enum FileFormat { ULP, CNF, LLS, INVALID };

// ULP
export type blocked = {
    [block: string]: {
        keys: { [key: string]: string },
        blocks: { [block: string]: string[] }
    }
};
function parseBlocked(file: string): blocked {
    let out: blocked = {};
    let lines = file.split("\n").map((s) => s.replace(/\s+/g, " ").replace(/\b\s+\b/g, "\t").replaceAll(" ", "").replaceAll("\t", " ")).filter((s) => s);
    let current_block = "", current_subblock = "";
    out[current_block] = { keys: {}, blocks: {} };
    for (let line of lines) {
        if (/^\[\w+\]$/.test(line)) {
            current_block = line.slice(1, line.length-1);
            out[current_block] = { keys: {}, blocks: {} };
            current_subblock = "";
        }
        else if (/^\w+:/.test(line)) {
            current_subblock = line.slice(0, line.indexOf(":"));
            out[current_block].blocks[current_subblock] = [];
        }
        else if (current_subblock == "") {
            out[current_block].keys[line.slice(0, line.indexOf("="))] = line.slice(line.indexOf("=")+1);
        }
        else {
            out[current_block].blocks[current_subblock].push(line);
        }
    }
    return out;
}

function parseClauseFunc(ulp: ULP, cf: string) {
    let [a, b, ...c] = cf.split(" ");
    if (b === "lookup_table") {
        let lut = ulp.addLookupTableClauseFunc(parseInt(a));
        if (c.length !== lut.table.length) throw new XLSError(`Incorrect number of looukp_table entries: expected ${lut.table.length}, got ${c.length}`);
        lut.table = c.map((i) => parseInt(i));
    }
    else {
        throw new XLSError(`Unrecognized clause func: ${b}`);
    }
}
function parseClause(ulp: ULP, clause: string) {
    let [a, ...b] = clause.split(" ");
    ulp.addClause(ulp.funcs[parseInt(a)], b.map((i) => parseInt(i)));
}
function parseOptions(dest: Options, options: { [key: string]: string }) {
    // TODO
}

export function loadULP(file: string): ULP {
    file = file.replace(/\r?\n|\r/g, "\n").replace(/#.*/gm, "");
    const blocks = parseBlocked(file);
    let ulp = new ULP(parseInt(blocks[""].keys.max_states));
    ulp.varCount = parseInt(blocks.GRAPH.keys.var_count);
    for (let cf of blocks.GRAPH.blocks.funcs) parseClauseFunc(ulp, cf);
    for (let clause of blocks.GRAPH.blocks.clauses) parseClause(ulp, clause);
    ulp.grid.expand({
        right: parseInt(blocks.GRID.keys.width),
        bottom: parseInt(blocks.GRID.keys.height),
        end: parseInt(blocks.GRID.keys.gens)
    }, new GridCell(0, undefined));
    for (let t = 0; t < ulp.grid.gens; t++) for (let y = 0; y < ulp.grid.height; y++) for (let x = 0; x < ulp.grid.width; x++) ulp.grid.set(t, x, y, new GridCell(parseInt(blocks.GRID.blocks.data[y+t*ulp.grid.height].split(" ")[x]), undefined));
    parseOptions(ulp.options, blocks.OPTIONS.keys);
    return ulp;
}

// LLS
export type LLS = string[][][];
export function loadLLS(file: string): LLS {
    file = file.replace(/\r?\n|\r/g, "\n").replace(/#.*/gm, "");
    let grid: LLS = file.split(/[ ,\t]*\n(?:[ ,\t]*\n)+[ ,\t]*/g).map((gen) => gen.split(/[ ,\t]*\n[ ,\t]*/g).map((row) => row.split(/[ ,\t]+/g)));
    if (!grid.every((a) => a.length === grid[0].length && a.every((b) => b.length === a[0].length))) throw new XLSError("LLS search pattern must be cuboidal");
    return grid;
}

// CNF
export type CNF = number[][];
export function saveCNF(file: CNF): string {
    let vars = 0, clauses = file.length;
    for (let clause of file) for (let lit of clause) {
        vars = vars > Math.abs(lit) ? vars : Math.abs(lit);
    }
    return `p cnf ${vars} ${clauses}\n` + file.map((clause) => clause.join(" ") + " 0").join("\n");
}
