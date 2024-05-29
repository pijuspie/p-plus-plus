var x = 24;
var y = 36;

var a = x;
var b = y;

while (a > 0 and b > 0) {
    if (b > a) {
        var c = b;
        b = a;
        a = c;
    }

    a = a - b;
}

print b;
print x * y / b;