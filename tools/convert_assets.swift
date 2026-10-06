/*
 * SWIFT  LIBRARY : Convert JPEG in ASSETS_DATA X STM32
*/

import CoreGraphics
import Foundation
import ImageIO



let targetWidth = 320
let targetHeight = 240
let pixelCount = targetWidth * targetHeight

func convertImageToRGB565Dithered(imageURL: URL) -> [UInt8]? {
    guard let source = CGImageSourceCreateWithURL(imageURL as CFURL, nil),
          let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else {
        fputs("Errore: impossibile caricare \(imageURL.lastPathComponent)
", stderr)
        return nil
    }

    let colorSpace = CGColorSpaceCreateDeviceRGB()
    let bytesPerRow = targetWidth * 4
    var rawBytes = [UInt8](repeating: 0, count: targetWidth * targetHeight * 4)

    guard let context = CGContext(data: &rawBytes,
                                  width: targetWidth,
                                  height: targetHeight,
                                  bitsPerComponent: 8,
                                  bytesPerRow: bytesPerRow,
                                  space: colorSpace,
                                  bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else {
        fputs("Errore: creazione fallita
", stderr)
        return nil
    }

    context.interpolationQuality = .high
    context.draw(image, in: CGRect(x: 0, y: 0, width: targetWidth, height: targetHeight))

    // 8x8 Bayer Ordered Dithering Matrix (Soglia spazialmente uniforme: nessun accumulo a destra)
    let bayer8x8: [Float] = [
         0.0/64.0, 32.0/64.0,  8.0/64.0, 40.0/64.0,  2.0/64.0, 34.0/64.0, 10.0/64.0, 42.0/64.0,
        48.0/64.0, 16.0/64.0, 56.0/64.0, 24.0/64.0, 50.0/64.0, 18.0/64.0, 58.0/64.0, 26.0/64.0,
        12.0/64.0, 44.0/64.0,  4.0/64.0, 36.0/64.0, 14.0/64.0, 46.0/64.0,  6.0/64.0, 38.0/64.0,
        60.0/64.0, 28.0/64.0, 52.0/64.0, 20.0/64.0, 62.0/64.0, 30.0/64.0, 54.0/64.0, 22.0/64.0,
         3.0/64.0, 35.0/64.0, 11.0/64.0, 43.0/64.0,  1.0/64.0, 33.0/64.0,  9.0/64.0, 41.0/64.0,
        51.0/64.0, 19.0/64.0, 59.0/64.0, 27.0/64.0, 49.0/64.0, 17.0/64.0, 57.0/64.0, 25.0/64.0,
        15.0/64.0, 47.0/64.0,  7.0/64.0, 39.0/64.0, 13.0/64.0, 45.0/64.0,  5.0/64.0, 37.0/64.0,
        63.0/64.0, 31.0/64.0, 55.0/64.0, 23.0/64.0, 61.0/64.0, 29.0/64.0, 53.0/64.0, 21.0/64.0
    ]

    var output = [UInt8](repeating: 0, count: pixelCount * 2)

    for y in 0..<targetHeight {
        for x in 0..<targetWidth {
            let idx = y * targetWidth + x
            let bayerIdx = (y % 8) * 8 + (x % 8)
            let threshold = bayer8x8[bayerIdx] - 0.5

            let rStep: Float = 255.0 / 31.0
            let gStep: Float = 255.0 / 63.0
            let bStep: Float = 255.0 / 31.0

            let rVal = max(0.0, min(255.0, Float(rawBytes[idx * 4])     + threshold * rStep))
            let gVal = max(0.0, min(255.0, Float(rawBytes[idx * 4 + 1]) + threshold * gStep))
            let bVal = max(0.0, min(255.0, Float(rawBytes[idx * 4 + 2]) + threshold * bStep))

            let r5 = UInt16(max(0.0, min(31.0, round((rVal / 255.0) * 31.0))))
            let g6 = UInt16(max(0.0, min(63.0, round((gVal / 255.0) * 63.0))))
            let b5 = UInt16(max(0.0, min(31.0, round((bVal / 255.0) * 31.0))))

            let rgb565 = (r5 << 11) | (g6 << 5) | b5
            output[idx * 2]     = UInt8((rgb565 >> 8) & 0xFF)
            output[idx * 2 + 1] = UInt8(rgb565 & 0xFF)
        }
    }

    return output
}

let cubeniroURL = URL(fileURLWithPath: "assets/cubeniro.jpeg")
let snakeURL = URL(fileURLWithPath: "assets/snakeSfondo.jpeg")

print("Conversione cubeniro.jpeg ")
guard let cubeniroData = convertImageToRGB565Dithered(imageURL: cubeniroURL) else { exit(1) }

print("Conversione snakeSfondo.jpeg ")
guard let snakeData = convertImageToRGB565Dithered(imageURL: snakeURL) else { exit(1) }

var cSource = """
#include "assets_data.h"
#include <stdint.h>

#if ENABLE_INTERNAL_ASSETS_DATA

const uint8_t asset_cubeniro_rgb565[153600] __attribute__((aligned(4))) = {

"""

func appendBytes(data: [UInt8], to str: inout String) {
    for i in 0..<data.count {
        if i % 16 == 0 {
            str += "  "
        }
        str += String(format: "0x%02XU, ", data[i])
        if i % 16 == 15 {
            str += "\n"
        }
    }
    if data.count % 16 != 0 {
        str += "\n"
    }
}

appendBytes(data: cubeniroData, to: &cSource)
cSource += "};\n\nconst uint8_t asset_snake_rgb565[153600] __attribute__((aligned(4))) = {\n"
appendBytes(data: snakeData, to: &cSource)
cSource += "};\n\n#endif /* ENABLE_INTERNAL_ASSETS_DATA */\n"

let destPath = "Core/Src/assets_data.c"
try! cSource.write(toFile: destPath, atomically: true, encoding: .utf8)
print("Generazione \(destPath) completata con successo!")
