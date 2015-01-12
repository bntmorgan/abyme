\documentclass{article}
\usepackage{fullpage}
\usepackage{listings}
\usepackage{amsfonts}
\usepackage{hyperref}
\usepackage{graphicx}
\usepackage[all]{hypcap}
\usepackage[utf8]{inputenc}
\usepackage[francais]{babel}
\usepackage{tikz}

\hypersetup{
  colorlinks,
  citecolor=black,
  filecolor=black,
  linkcolor=black,
  urlcolor=black
}

\definecolor{bgblue}{rgb}{0.41961,0.80784,0.80784}%
\definecolor{bgred}{rgb}{1,0.61569,0.61569}%
\definecolor{fgblue}{rgb}{0,0,0.6}%
\definecolor{fgred}{rgb}{0.6,0,0}%
\definecolor{dkgreen}{rgb}{0,0.6,0}
\definecolor{gray}{rgb}{0.5,0.5,0.5}
\definecolor{mauve}{rgb}{0.58,0,0.82}

\lstset{ %
  literate=%
     {à}{{\`a}}1
     {â}{{\^a}}1
     {é}{{\'e}}1
     {ê}{{\^e}}1
     {è}{{\`e}}1,
  language=C,                       % the language of the code
  basicstyle=\footnotesize,         % the size of the fonts that are used for the code
  numbers=left,                     % where to put the line-numbers
  numberstyle=\tiny\color{gray},    % the style that is used for the line-numbers
  stepnumber=1,                     % the step between two line-numbers. If it's 1, each line 
                                    % will be numbered
  numbersep=5pt,                    % how far the line-numbers are from the code
  backgroundcolor=\color{white},    % choose the background color. You must add \usepackage{color}
  showspaces=false,                 % show spaces adding particular underscores
  showstringspaces=false,           % underline spaces within strings
  showtabs=false,                   % show tabs within strings adding particular underscores
  frame=single,                     % adds a frame around the code
  rulecolor=\color{black},          % if not set, the frame-color may be changed on
                                    % line-breaks within not-black text (e.g. commens (green here))
  tabsize=2,                        % sets default tabsize to 2 spaces
  captionpos=b,                     % sets the caption-position to bottom
  breaklines=true,                  % sets automatic line breaking
  breakatwhitespace=false,          % sets if automatic breaks should only happen at whitespace
  title=\lstname,                   % show the filename of files included with \lstinputlisting;
                                    % also try caption instead of title
  keywordstyle=\color{blue},        % keyword style
  commentstyle=\color{dkgreen},     % comment style
  stringstyle=\color{mauve},        % string literal style
  escapeinside={\%*}{*)},           % if you want to add a comment within your code
  morekeywords={*,vector, iterator} % if you want to add more keywords to the set
}

\title{SVC-hyperviseur documentation développeur}

\begin{document}

\maketitle 

\tableofcontents 

\def\figwidth{12cm}

@i multicore.w

\end{document}
