package antlr;

/**
 * <b>SOFTWARE RIGHTS</b>
 * <p>
 * ANTLR 2.6.0 MageLang Institute
 * <p>
 * We reserve no legal rights to the ANTLR--it is fully in the
 * public domain. An individual or company may do whatever
 * they wish with source code distributed with ANTLR or the
 * code generated by ANTLR, including the incorporation of
 * ANTLR, or its output, into commerical software.
 * <p>
 * We encourage users to develop software with ANTLR. However,
 * we do ask that credit is given to us for developing
 * ANTLR. By "credit", we mean that if you use ANTLR or
 * incorporate any source code into one of your programs
 * (commercial product, research project, or otherwise) that
 * you acknowledge this fact somewhere in the documentation,
 * research report, etc... If you like ANTLR and have
 * developed a nice tool with the output, please mention that
 * you developed it using ANTLR. In addition, we ask that the
 * headers remain intact in our source code. As long as these
 * guidelines are kept, we expect to continue enhancing this
 * system and expect to make other tools available as they are
 * completed.
 * <p>
 * The ANTLR gang:
 * @version ANTLR 2.6.0 MageLang Institute
 * @author Terence Parr, <a href=http://www.MageLang.com>MageLang Institute</a>
 * @author <br>John Lilley, <a href=http://www.Empathy.com>Empathy Software</a>
 */
public class ParserException extends ANTLRException {
	public int line;	// not used by treeparsers
	public int column;	// not used by treeparsers

	public ParserException() {
		super("parsing error");
	}
	public ParserException(String m) {
		super(m);
	}
		/**
	 * @return the column number that this exception happened on.
	 * @author Shawn P. Vincent (svincent@svincent.com)
	 */
		public int getColumn() { return column; }
		public String getErrorMessage () { return toString (); }
		/**
	 * @return the line number that this exception happened on.
	 * @author Shawn P. Vincent (svincent@svincent.com)
	 */
		public int getLine() { return line; }
}