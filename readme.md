# P++ Programming Language

This is an interpreter for my language called p++.
It is written in c++. It has a Mark-Sweep Garbage Collector.
More than 2500 lines of code.

## Usage

Compile the interpreter:
```
build.cmd
```
Run REPL (code execution line by line):
```
p++ 
```
Run a code file:
```
p++ file_name.p
```

## Syntax

#### Variables

```
var a = 5;
var b = 2.5;
var c = "hello";
var a2 = true;

a = a + 1;
var d = a * b;
var e = 123 % 11;
```

#### Control Flow

```
var a = 15;

if (a >= 10) {
    a = 10;
} else if (a < 0) {
    a = 0;
}

while (a < 10) {
    a++;
}

var c = 1;
for (var b = 0; b < a; b = b + 1) {
    c = c * 2;
}
```

#### Functions

```
fun sum(a, b, c) {
    return a+b+c;
}

var sum1 = 4 + sum(5, 6, 7);

fun out(x, y) {
    printl x+" "+y;
}

out("Hello", "world!");
```

#### Classes

```
class myClass {
    init(a, b) {
        this.a = a;
        this.b = b;
    }

    sum() {
        return a+b;
    }

    sub() {
       return a-b; 
    }
}

var myInstance = myClass(10, 7);
printl myInstance.sum();

var sub = myInstance.sub();
myInstance.a = 15;
printl sub();
```

#### Other

Native functions:
* `clock()` - gives the elapsed time since the program started running, in seconds
* `readNumber()` - reads user input, converts it to number and returns (returns zero if fails to do so)
* `stringify(x)` - converts x to string
* `round(x, y)` - rounds x value to the closest multiple of y

```
var begin = clock();

var a = "Hello world!";
print a;

print "Number x:";
var x = readNumber();

var k = 85.43899;
var l = 100;
for (var i = 0; i<9; i = i+1) {
    printl "k: " + stringify(round(k, l));
    l = l / 10;
}

var end = clock();
print end-begin;
printl " seconds";
```

## Examples

Greatest common divider and lowest common multiple:

```
p++ examples/gcd-lcm.p
```

Function closures:

```
p++ examples/closures.p
```

The Fibonacci sequence (using recursion):

```
p++ examples/fibonacci.p
```

Pi calculator:

```
p++ examples/pi.p
```

Another example with classes:

```
p++ examples/class.p
```

## License

Copyright © 2024, pijuspie
