const test = require('brittle')
const tiff = require('.')

test('decode .tiff', (t) => {
  const image = require('./test/fixtures/grapefruit.tiff', {
    with: { type: 'binary' }
  })

  t.comment(tiff.decode(image))
})

test('encode .tiff', (t) => {
  const image = require('./test/fixtures/grapefruit.tiff', {
    with: { type: 'binary' }
  })

  const decoded = tiff.decode(image)

  t.comment(tiff.encode(decoded))
})
