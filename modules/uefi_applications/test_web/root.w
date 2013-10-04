\documentclass{article}
  \usepackage{listings}
  \usepackage{amsfonts}
  \usepackage{hyperref}
  \usepackage[all]{hypcap}
\begin{document}
\section{TEST}

A comment 1.  A comment 2.  A comment x.  A comment x.  A comment x.  A comment
x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A
comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment
x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A
comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment
x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A
comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment
x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A
comment x.  A comment x.  A comment x.  A comment x.  A comment x.  A comment
x.

\subsection{SUBSECTION}
@++ code/test.c
@< imports of packages >
void main(void) {
  @< delcarations >
  @< calcul >
}
@-

\subsection{SUBSECTION}

@++ code/file.py
@< imports of packages >
  print math.pi, time.time()
123 @< value > 456
123 @< value > 456
123 @< value > 456
123 @< value > 456
print math.pi, time.time()
print math.pi, time.time()
print math.pi, time.time()
print math.pi, time.time()
@< imports of packages >
@-
  
@+ calcul
  a = @< value >;
  b = a * 2;
  printf("%d\n", a);
  printf("%d\n", b);
@-

\section{SECTION}

Another comment 1.  Another comment 2.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.

Another comment 1.  Another comment 2.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.


@+ value
111@-

Another comment 1.  Another comment 2.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.

Include a file...
@i next_file.w

End of file 1.
End of file 2.

Another comment 1.  Another comment 2.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.

@l

Another comment 1.  Another comment 2.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.  Another comment x.  Another comment x.  Another comment x.
Another comment x.

\end{document}
