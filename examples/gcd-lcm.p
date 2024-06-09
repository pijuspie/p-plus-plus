print "Number x: ";
var x = readNumber();
print "Number y: ";
var y = readNumber();

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

print "GCD: ";
print b;
print "\nLCM: ";
print x * y / b;
