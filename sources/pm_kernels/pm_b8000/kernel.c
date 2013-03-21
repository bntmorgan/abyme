int main() {
  unsigned short *ptr;
  ptr = (unsigned short *) 0xa0000;
  unsigned short int i;
  for (i = 0; i < 204800; i++){
    ptr[i] = i;
  }
  while (1);
  return 0;
}
