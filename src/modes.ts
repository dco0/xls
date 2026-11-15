
import {ULP} from './base.js';
import {runScript} from './scripts.js';


export function periodicMode(dx: number, dy: number, period: number, width: number, height: number): string {
    return '\n' + `
grid.width = ${width}
grid.height = ${height}
grid.gens = ${period}
grid.apply_wrap(${dx}, ${dy})
    `.trim() + '\n';
}
