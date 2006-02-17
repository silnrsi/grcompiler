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
import antlr.collections.AST;

/** Common AST node implementation */
public class CommonAST extends BaseAST {
	int ttype = Token.INVALID_TYPE;
	String text;


	/** Get the token text for this node */
	public String getText() { return text; }
	/** Get the token type for this node */
	public int getType() { return ttype; }
	public void initialize(int t, String txt) {
		setType(t);
		setText(txt);
	}
public void initialize(AST t) {
	setText(t.getText());
	setType(t.getType());
}
public void initialize(Token tok) {
	setText(tok.getText());
	setType(tok.getType());
}
	/** Set the token text for this node */
	public void setText(String text_) { 
		text = text_; 
	}
	/** Set the token type for this node */
	public void setType(int ttype_) { 
		ttype = ttype_; 
	}
}