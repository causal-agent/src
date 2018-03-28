// cargo-deps: hsl
extern crate hsl;

fn main() {
    for i in 0..256 {
        let h = i as f64 * 360.0 / 256.0;
        let hsl = hsl::HSL { h, s: 1.0, l: 0.5 };
        let (r, g, b) = hsl.to_rgb();
        println!("0x{:02X}{:02X}{:02X},", r, g, b);
    }
}
