import { writeFileSync } from "fs";
import { cfns } from "./cfunction";

export function generateReadme() {
  const readme =
`# Mojibake

Mojibake is a low-level Unicode library written in C99.

## API

${cfns.map(value => value.formatMD()).join('\n\n')}
`;

  writeFileSync('../README.md', readme);
}
