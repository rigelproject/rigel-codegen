// RUN: %llvmgxx -g -c %s
struct s8_0 { unsigned : 0; };
struct s8_1 { double x; };
struct s8 { s8_0 a; s8_1 b; };
s8 f8() { return s8(); }
