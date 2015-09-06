function getOM
  input String n;
  output Real y;

  external y = getOM(n) annotation(Library = "/home/michele/Developer/Modelica/Progetto/SocketLibrary/lib/libSocketsModelica.a", Include = "#include \"/home/michele/Developer/Modelica/Progetto/SocketLibrary/include/libSocketsModelica.h\"");
end getOM;