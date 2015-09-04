function sendOM
  input Real x;
  input String n;

  external sendOM(x, n) annotation(Library = "../SocketLibrary/libSocketsModelica.a", Include = "#include \"../SocketLibrary/libSocketsModelica.h\"");
end sendOM;