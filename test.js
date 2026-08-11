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

test('decode rejects overflowing dimensions', (t) => {
  // Minimal little-endian TIFF that satisfies libtiff's required-tag check but
  // declares ImageWidth = ImageLength = 0xFFFFFFFF, so width * height * 4
  // overflows size_t — the pre-fix heap-overflow trigger.
  const evil = Buffer.from(
    '49492a00' + // II*\0 header
      '08000000' + // IFD offset = 8
      '0500' + // 5 directory entries
      '0001040001000000ffffffff' + // ImageWidth (256), LONG, count 1, 0xFFFFFFFF
      '0101040001000000ffffffff' + // ImageLength (257), LONG, count 1, 0xFFFFFFFF
      '110104000100000000000000' + // StripOffsets (273), LONG, count 1, 0
      '1601040001000000ffffffff' + // RowsPerStrip (278), LONG, count 1, 0xFFFFFFFF
      '170104000100000000000000' + // StripByteCounts (279), LONG, count 1, 0
      '00000000', // next IFD = 0
    'hex'
  )

  t.exception(() => tiff.decode(evil), /invalid image dimensions/i)
})

test('decode throws on non-TIFF input', (t) => {
  t.exception(() => tiff.decode(Buffer.from('this is not a tiff')))
})

test('decode throws on truncated TIFF', (t) => {
  // Valid little-endian magic + IFD offset pointing past the end of the buffer,
  // so TIFFClientOpen fails to read the directory.
  const truncated = Buffer.from('49492a00' + '08000000', 'hex')

  t.exception(() => tiff.decode(truncated))
})

test('encode rejects zero width', (t) => {
  t.exception(
    () => tiff.encode({ data: new Uint8Array(4), width: 0, height: 1 }),
    /invalid image dimensions/i
  )
})

test('encode rejects zero height', (t) => {
  t.exception(
    () => tiff.encode({ data: new Uint8Array(4), width: 1, height: 0 }),
    /invalid image dimensions/i
  )
})

test('encode rejects negative dimensions', (t) => {
  t.exception(
    () => tiff.encode({ data: new Uint8Array(4), width: -1, height: 1 }),
    /invalid image dimensions/i
  )
})

test('encode rejects dimensions exceeding uint32', (t) => {
  t.exception(
    () => tiff.encode({ data: new Uint8Array(4), width: 0x100000000, height: 1 }),
    /invalid image dimensions/i
  )
})

test('encode rejects buffer smaller than dimensions', (t) => {
  t.exception(
    () => tiff.encode({ data: new Uint8Array(10), width: 100, height: 100 }),
    /buffer too small/i
  )
})
