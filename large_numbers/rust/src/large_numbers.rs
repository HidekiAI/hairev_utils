
pub fn do_something(s: &str) {
    println!("do_something {s}");
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::large_numbers;

    #[test]
    fn it_works() {
        large_numbers::do_something("lib test");
    }
}

