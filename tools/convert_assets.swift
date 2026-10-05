import CoreGraphics
import Foundation
import ImageIO

let arguments = CommandLine.arguments
if arguments.count != 3 {
    fputs("usage: convert_assets input.jpeg output.rgb565\n", stderr)
    exit(2)
}

let inputURL = URL(fileURLWithPath: arguments[1])
let outputURL = URL(fileURLWithPath: arguments[2])
guard let source = CGImageSourceCreateWithURL(inputURL as CFURL, nil),
      let image = CGImageSourceCreateImageAtIndex(source, 0, nil),
      let context = CGContext(data: nil,
                              width: 128,
                              height: 160,
                              bitsPerComponent: 8,
                              bytesPerRow: 128 * 4,
                              space: CGColorSpaceCreateDeviceRGB(),
                              bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue),
      let data = context.data else {
    fputs("cannot decode image\n", stderr)
    exit(1)
}

context.interpolationQuality = .high
context.draw(image, in: CGRect(x: 0, y: 0, width: 128, height: 160))
let pixels = data.bindMemory(to: UInt8.self, capacity: 128 * 160 * 4)
var rgb565 = Data(capacity: 128 * 160 * 2)
for index in 0..<(128 * 160) {
    let red = UInt16(pixels[index * 4])
    let green = UInt16(pixels[index * 4 + 1])
    let blue = UInt16(pixels[index * 4 + 2])
    let value = (red >> 3) << 11 | (green >> 2) << 5 | (blue >> 3)
    rgb565.append(UInt8(value >> 8))
    rgb565.append(UInt8(value & 0xFF))
}
try! rgb565.write(to: outputURL)
