function sendOM
  input Real x;
  input String n;

  external sendOM(x, n) annotation(Library = "/home/michele/Developer/Modelica/Progetto/SocketLibrary/lib/libSocketsModelica.a", Include = "#include \"/home/michele/Developer/Modelica/Progetto/SocketLibrary/include/libSocketsModelica.h\"");
end sendOM;