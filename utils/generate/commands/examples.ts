import functions from '../functions';
import { CFunction } from '../html-function';

function generateExample(name: string, args: string[], after?: string): string {
    const fn = functions.find(f => f.name === name);

    if(!fn) {
        throw new Error(`Function ${name} not found`);
    }

    const htmlFn = new CFunction(fn);

    /*return `<pre><code class="hljs language-c">${
        hljs.highlight(htmlFn.functionCallHTML(args, after), { language: 'c' }).value
    }</code></pre>`;*/
    return htmlFn.functionCallHTML(args, after);
}

function generateExamples(): void {
    console.log(generateExample('mjb_codepoint_info', ['0x022A', '&character']));
}

generateExamples();
