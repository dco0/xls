
import {ULP} from './base.js';
import {runScript} from './scripts.js';


export async function periodicMode(dx: number, dy: number, period: number, width: number, height: number): Promise<ULP> {
    return await runScript(`
        grid.width = ${width}
        grid.height = ${height}
        grid.gens = ${period}
        grid.apply_wrap(${dx}, ${dy})
    `);
}
