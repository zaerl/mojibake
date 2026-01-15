/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

export class CStruct {
  #ptr;
  #offset = 0;

  constructor(ptr) {
    this.#ptr = ptr;
    this.#offset = 0;
  }

  u8(heap8) {
    const ret = heap8[this.#ptr + this.#offset];
    this.#offset += 1;
    return ret;
  }

  u16(heap8) {
    // Ensure 2-byte alignment
    this.#offset = Math.ceil(this.#offset / 2) * 2;
    const ret = new DataView(heap8.buffer, this.#ptr + this.#offset).getUint16(0, true);
    this.#offset += 2;

    return ret;
  }

  u32(heap8) {
    // Ensure 4-byte alignment
    this.#offset = Math.ceil(this.#offset / 4) * 4;
    const ret = new DataView(heap8.buffer, this.#ptr + this.#offset).getUint32(0, true);
    this.#offset += 4;

    return ret;
  }

  i32(heap8) {
    // Ensure 4-byte alignment
    this.#offset = Math.ceil(this.#offset / 4) * 4;
    const ret = new DataView(heap8.buffer, this.#ptr + this.#offset).getInt32(0, true);
    this.#offset += 4;

    return ret;
  }

  str(heap8, max) {
    const nameBytes = new Uint8Array(heap8.buffer, this.#ptr + this.#offset, max);
    let end = nameBytes.indexOf(0);

    if(end === -1) {
        end = max;
    }

    this.#offset += max;

    return new TextDecoder().decode(nameBytes.subarray(0, end));
  }

  toObject(signature, heap8) {
    const object = {};

    for(const name in signature) {
      if(signature[name].startsWith('str')) {
        object[name] = this.str(heap8, Number(signature[name].split('str')[1]));
      } else {
        object[name] = this[signature[name]](heap8);
      }
    }

    return object;
  }
}
