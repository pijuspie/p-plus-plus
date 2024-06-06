# P++ Programming Language

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
p++ failo_pavadinimas.p
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
    print x+" "+y;
}

out("Hello", "world!");
```

#### Other

```
var begin = clock();

var a = "Hello world!";
print a;

print "Number x:";
var x = readNumber();

var end = clock();
print end-begin;
print "Seconds";
```

## Examples

Greatest common divider and lowest common multiple:

```
p++ examples/gcd-lcm.p
```
