var a = 24;
var b = 36;

while (a > 0 and b > 0) {
    if (b > a) {
        var c = b;
        b = a;
        a = c;
    }

    a = a - b;
}

print b;