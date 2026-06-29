import createMojibake from '/api/index.js';

const mojibake = await createMojibake();
console.log(mojibake.version());
