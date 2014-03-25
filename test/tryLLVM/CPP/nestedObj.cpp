#include <cstdio>
class Circle {
  double radius;
public:
  Circle(double r) : radius(r) { }
  double area() {return radius*radius*3.14159265;}
};

class Cylinder {
  Circle *base;
  double height;
public:
  Cylinder(double r, double h) : height(h) {
    base = new Circle(r);
    printf("base at %x \n", base);
  }
  double volume() {
    printf("base at %x read \n", base);
    return base->area() * height;
  }
  ~Cylinder() {
    delete base;
  }
};

int main () {
  Cylinder *foo = new Cylinder(10,20);
  printf("foo at %x \n", foo);
  delete foo;

  return 0;
}
