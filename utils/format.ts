export function header(name: string): string {
  name = name.toUpperCase();

  return `${license()}

#ifndef MJB_${name}_H
#define MJB_${name}_H`;
}

export function footer(name: string): string {
  name = name.toUpperCase();

  return `#endif /* MJB_${name}_H */`;
}

export function license(): string {
  return `/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */`;
}
