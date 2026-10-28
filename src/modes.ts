
import {ULP} from './base.js';
import {runScript} from './scripts.js';


export async function periodicMode(dx: number, dy: number, period: number, width: number, height: number): Promise<ULP> {
    return await runScript(`
        grid.width = ${width}
        grid.height = ${height}
        grid.gens = ${period}
        for coord in (0, 0, 0)...(0, grid.width, grid.height) {
            let mapped = (grid.gens - 1, coord.x + ${dx}, coord.y + ${dy})
            if (in_bounds(mapped)) {
                grid[coord] = grid[mapped]
            }
        }
    `);
}
