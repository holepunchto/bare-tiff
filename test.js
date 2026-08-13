const test = require('brittle')
const tiff = require('.')

// TIFF directory entry types
const SHORT = 3
const LONG = 4

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

test('decode throws when a strip points outside the file', (t) => {
  // Opens cleanly - every required tag is there - but StripOffsets sends
  // libtiff past the end of the buffer, so it fails while reading pixels.
  const image = buildTIFF([
    { tag: 0x0100, type: LONG, value: 4 },
    { tag: 0x0101, type: LONG, value: 4 },
    { tag: 0x0102, type: SHORT, value: 8 },
    { tag: 0x0103, type: SHORT, value: 1 },
    { tag: 0x0106, type: SHORT, value: 1 },
    { tag: 0x0111, type: LONG, value: 100000 }, // StripOffsets, past the end
    { tag: 0x0115, type: SHORT, value: 1 },
    { tag: 0x0116, type: LONG, value: 4 },
    { tag: 0x0117, type: LONG, value: 16 }
  ])

  t.exception(() => tiff.decode(image), /Seek error/i)
})

test('encode rejects dimensions whose product overflows', (t) => {
  // Each side is exactly UINT32_MAX, so they clear the per-side check; it is
  // width * height * 4 that does not fit in a size_t.
  t.exception(
    () =>
      tiff.encode({
        data: new Uint8Array(4),
        width: 0xffffffff,
        height: 0xffffffff
      }),
    /invalid image dimensions/i
  )
})

// Helpers

function buildTIFF(entries) {
  const header = Buffer.alloc(8)

  header.write('II', 0, 'ascii')
  header.writeUInt16LE(42, 2)
  header.writeUInt32LE(8, 4) // offset of the first directory

  const ifd = Buffer.alloc(2 + entries.length * 12 + 4)

  ifd.writeUInt16LE(entries.length, 0)

  entries.forEach(({ tag, type, value }, i) => {
    const at = 2 + i * 12

    ifd.writeUInt16LE(tag, at)
    ifd.writeUInt16LE(type, at + 2)
    ifd.writeUInt32LE(1, at + 4) // count

    if (type === SHORT) ifd.writeUInt16LE(value, at + 8)
    else ifd.writeUInt32LE(value, at + 8)
  })

  return Buffer.concat([header, ifd])
}
