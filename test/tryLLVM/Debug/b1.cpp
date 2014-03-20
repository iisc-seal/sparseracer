int main(){
  int a, b, c;
  int *ptr;
  ptr = new int[10];
  ptr[1] = 5;
  delete[] ptr;
  b = 6, c = 7;
  a = b + c;
}
