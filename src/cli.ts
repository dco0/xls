
import {XLSError} from './base.js'


if (import.meta.main) {
    try {
        await import('./run_cli.js')
    } catch (error) {
        if (error instanceof XLSError) {
            console.log(`${error.message}\nUse ./xls -h for help`);
            process.exit(1);
        } else {
            throw error;
        }
    }
}
