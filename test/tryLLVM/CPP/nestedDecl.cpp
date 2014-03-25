// nested_class_declarations_2.cpp
class C
{
public:
  typedef class U u_t; // class U visible outside class C scope
  typedef class V {} v_t; // class V not visible outside class C
};

int main()
{
  // okay, forward declaration used above so file scope is used
  U* pu;

  // error, type name only exists in class C scope
  u_t* pu2; // C2065

  // error, class defined above so class C scope
  V* pv; // C2065

  // okay, fully qualified name
  C::V* pv2;
}
