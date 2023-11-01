#include <stdio.h>

// Declare a global variable
int global_var = 0;

// Declare a global constructor
struct GlobalConstructor {
  GlobalConstructor() {
    global_var = 10;
  }
};

// Define the global constructor
GlobalConstructor global_constructor;

class Shape {
public:
    virtual float getArea() = 0; // pure virtual function
};

class Rectangle : public Shape {
private:
    float width, height;

public:
    Rectangle(float w, float h) : width(w), height(h) {}

    float getArea() {
        return width * height;
    }
};

class Circle : public Shape {
private:
    float radius;

public:
    Circle(float r) : radius(r) {}

    float getArea() {
        return 3.14159 * radius * radius;
    }
};

int main(void) {
    Shape *shape1 = new Rectangle(10, 5);
    Shape *shape2 = new Circle(7);

    // iosteam cout takes more memory, I use printf here.
    printf("%f\n", shape1->getArea()+shape2->getArea()+global_var);

    return 0;
}

