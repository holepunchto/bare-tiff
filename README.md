# bare-tiff

TIFF support for Bare.

```
npm i bare-tiff
```

## Usage

```js
const tiff = require('bare-tiff')

const image = require('./my-image.tiff', { with: { type: 'binary' } })

const decoded = tiff.decode(image)
// {
//   width: 200,
//   height: 400,
//   data: <Buffer>
// }

const encoded = tiff.encode(decoded)
// <Buffer>
```

## License

Apache-2.0
