\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{homework}[2015/11/29]

%% Optionen, die an article.cls weitergegeben werden sollen...
\DeclareOption{12pt,a4paper}{
  \PassOptionsToClass{\CurrentOption}{article}
}

%% Fallback, falls ein Parameter unbekannt ist...
\DeclareOption*{
  \ClassWarning{homework}{Unknown option '\CurrentOption'}
}

%% Default options...
\ExecuteOptions{12pt}

%% Process given options...
\ProcessOptions\relax

%% Superklasse laden...
\LoadClass[12pt,a4paper]{homework}

%% Abhängigkeiten laden...
\RequirePackage[utf8]{inputenc}
\RequirePackage[english]{babel}
\RequirePackage[T1]{fontenc}
\RequirePackage{graphicx}
\RequirePackage{amsfonts}
\RequirePackage{amssymb}
\RequirePackage{lmodern}
\RequirePackage{amsmath}
\RequirePackage{paralist}
\RequirePackage{tabularx}
\RequirePackage{delarray}
\RequirePackage{fancyhdr}
\RequirePackage{mathtools}
\RequirePackage{wasysym}
\RequirePackage{colortbl}
\RequirePackage{amsmath}
\RequirePackage{graphicx}
\RequirePackage{amsfonts}
\RequirePackage{amssymb}

\author{Fabian Schmidt \texttt{<fa.schmidt@tuhh.de>} \\Fin T\"oter \texttt{<f.toeter@gmail.com>} \\Thomas Pohl \texttt{<thomas.pohl@tuhh.de>} \\Nicolai Frech \texttt{<nicolai.frech@tuhh.de>} }
\course{Prozedurale Programmierung WS 2015/16}
\subsectioncounter{\alph{subsection}}
\subsectionsuffix{)}
\subsubsectioncounter{(\roman{subsubsection})}
\subsubsectionsuffix{}
%\group{01}
\fancyhead[L]{\thedate}
\courseprefix{}
\sheetprefix{}
\sectionprefix{Teil }

\endinput
