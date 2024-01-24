class Example {
  public int [] p;
  
  public void foo(int a)
  {
    int c = 0;
    
    if (p == null)
      c = a;
    
    c += p[a];
  }
}
