function getOM
  input String n;
  output Real y;

  external y = getOM(n) annotation(Library = "../SocketLibrary/libSocketsModelica.a", Include = "#include \"../SocketLibrary/libSocketsModelica.h\"");
end getOM;