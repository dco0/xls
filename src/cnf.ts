import { type ULP, XLSError } from "./base.js";
import { type CNF } from "./fileformats.js";

type encoding = [number, number][];

// variable i in ULP maps to i-1 in CNF
export function ULP_to_CNF(ulp: ULP): CNF {
    if (ulp.maxStates !== 2) throw new XLSError("max_states > 2 is not supported");
    let funcs = ulp.funcs.map((func) => getPrimes(func.varCount, (i) => func.data[i] !== 0));
    return ulp.clauses.flatMap((clause) => createProp(clause.data.map((v) => v ? v : -1), funcs[ulp.funcs.indexOf(clause.func)]));
}

function createProp(vars: number[], encoding: encoding): CNF {
    let cnf: CNF = [];
    outer: for (let cube of encoding) {
        let clause: number[] = [];
        for (let i = 0; i < vars.length; i++) {
            let v = vars[i];
            if (cube[1] >> i & 1) continue;
            let negated = (cube[0] >> i & 1) ? -1 : 1;
            let lit = negated * (v > 0 ? v-1 : v+1);
            if (lit !== 0) { if (!clause.includes(lit)) clause.push(lit); }
            else if (v === negated) continue outer;
        }
        if (cnf.every((i) => !i.every((j) => clause.includes(j))) && clause.every((i) => !clause.includes(-i)))
            cnf.push(clause);
    }
    return cnf.filter((k) => cnf.every((i) => i.length === k.length || !i.every((j) => k.includes(j))));
}

function getPrimes(vars: number, fn: (i: number) => boolean): encoding {
    let t = Array.from({ length: 1 << vars }, (_, i) => [i, 0, 0] as [number, number, number]).filter((x) => !fn(x[0]));
    for (let i = 0; i < t.length; i++) for (let j = 0; j < t.length; j++) {
        let diff = t[i][0] ^ t[j][0];
        if (diff && !(diff & diff-1)) t[i][2] |= diff;
    }
    let out: encoding = [];
    for (let v = 0; v <= vars; v++) {
        let s: typeof t = [];
        for (let i = 0, j = 0; i < t.length; i++) {
            if (t[i][2] == 0) out.push([t[i][0], t[i][1]]);
            else {
                if (t[i][2]>>(v+1) != 0) s.push(t[i]);
                if ((t[i][0]>>v & 1) == 0 && (t[i][2]>>v & 1) == 1) {
                    while ((t[i][0] | 1<<v) != t[j][0] || t[i][1] != t[j][1]) j++;
                    s.push([t[i][0], t[i][1] | 1<<v, t[i][2] & t[j][2] & ~(1<<v)]);
                }
            }
        }
        t = s;
    }
    return out;
}
