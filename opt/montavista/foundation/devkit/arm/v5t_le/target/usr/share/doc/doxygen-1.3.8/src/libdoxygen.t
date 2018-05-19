#
# $Id: libdoxygen.t,v 1.1 2001/03/19 19:27:41 root Exp $
#
# Copyright (C) 1997-2004 by Dimitri van Heesch.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation under the terms of the GNU General Public License is hereby 
# granted. No representations are made about the suitability of this software 
# for any purpose. It is provided "as is" without express or implied warranty.
# See the GNU General Public License for more details.
#
# Documents produced by Doxygen are derivative works derived from the
# input used in their production; they are not affected by this license.
#!
#! doxygen.t: This is a custom template for building Doxygen
#!
#$ IncludeTemplate("lib.t");

LEX        = flex
YACC       = bison
INCBUFSIZE = $(PERL) increasebuffer.pl

#${
sub GenerateDep {
    my($obj,$src,$dep) = @_;
    my(@objv,$srcv,$i,$s,$o,$d,$c);
    @objv = split(/\s+/,$obj);
    @srcv = split(/\s+/,$src);
    for $i ( 0..$#objv ) {
        $s = $srcv[$i];
        $o = $objv[$i];
        next if $s eq "";
        $text .= $o . ": " . $s;
	$text .= " ${linebreak}\n\t\t" . $dep if $dep ne "";
        if ( $moc_output{$s} ne "" ) {
            $text .= " ${linebreak}\n\t\t" . $moc_output{$s};
        }
        $d = &make_depend($s);
        $text .= " ${linebreak}\n\t\t" . $d if $d ne "";
	$text .= "\n";
    }
    chop $text;
}
#$} 

####################

#$ GenerateDep("scanner.cpp","scanner.l");
	$(LEX) -PscanYY -t scanner.l | $(INCBUFSIZE) >scanner.cpp

#$ GenerateDep("code.cpp","code.l");
	$(LEX) -PcodeYY -t code.l | $(INCBUFSIZE) >code.cpp

#$ GenerateDep("pre.cpp","pre.l");
	$(LEX) -PpreYY -t pre.l | $(INCBUFSIZE) >pre.cpp 

#$ GenerateDep("declinfo.cpp","declinfo.l");
	$(LEX) -PdeclinfoYY -t declinfo.l >declinfo.cpp

#$ GenerateDep("defargs.cpp","defargs.l");
	$(LEX) -PdefargsYY -t defargs.l >defargs.cpp

#$ GenerateDep("doctokenizer.cpp","doctokenizer.l");
	$(LEX) -PdoctokenizerYY -t doctokenizer.l >doctokenizer.cpp

#$ GenerateDep("commentcnv.cpp","commentcnv.l");
	$(LEX) -PcommentcnvYY -t commentcnv.l >commentcnv.cpp

#$ GenerateDep("ce_lex.cpp","constexp.l","ce_parse.h");
	$(LEX) -PcppExpYY -t constexp.l >ce_lex.cpp 

#$ GenerateDep("ce_parse.cpp","constexp.y");
	$(YACC) -l -p cppExpYY constexp.y -o ce_parse.cpp 

#$ GenerateDep("ce_parse.h","constexp.y");
	$(YACC) -l -d -p cppExpYY constexp.y -o ce_parse.c 
	-rm ce_parse.c	

index_xsd.h: index.xsd
	cat index.xsd | sed -e "s/\\\\/\\\\\\\\/g" -e "s/\"/\\\\\"/g" -e "s/^/\"/g" -e "s/$$/\\\\n\"/g" >index_xsd.h 

compound_xsd.h: compound.xsd
	cat compound.xsd | sed -e "s/\\\\/\\\\\\\\/g" -e "s/\"/\\\\\"/g" -e "s/^/\"/g" -e "s/$$/\\\\n\"/g" >compound_xsd.h 

search_php.h: search.php
	cat search.php | sed -e "s/\\\\/\\\\\\\\/g" -e "s/\"/\\\\\"/g" -e "s/^/\"/g" -e "s/$$/\\\\n\"/g" >search_php.h 

