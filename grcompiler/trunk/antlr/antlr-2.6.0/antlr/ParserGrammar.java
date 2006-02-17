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

import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;
import antlr.collections.impl.BitSet;
import antlr.collections.impl.Vector;


/** Parser-specific grammar subclass */
class ParserGrammar extends Grammar {


	ParserGrammar(String className_, Tool tool_, String superClass) {
		super(className_, tool_, superClass);
	}
	/** Top-level call to generate the code for this grammar */
	public void generate() throws IOException {
		generator.gen(this);
	}
	// Get name of class from which generated parser/lexer inherits
	protected String getSuperClass() {
		// if debugging, choose the debugging version of the parser
		if (debuggingOutput)
			return "debug.LLkDebuggingParser";
		return "LLkParser"; 
	}
	/**Process command line arguments.
	 * -trace			have all rules call traceIn/traceOut
	 * -traceParser		have parser rules call traceIn/traceOut
	 * -debug			generate debugging output for parser debugger
	 */
	public void processArguments(String[] args) {
		for (int i=0; i<args.length; i++) {
			if ( args[i].equals("-trace") ) {
				traceRules = true;
				Tool.setArgOK(i);
			}
			else if ( args[i].equals("-traceParser") ) {
				traceRules = true;
				Tool.setArgOK(i);
			}
			else if ( args[i].equals("-debug") ) {
				debuggingOutput = true;
				Tool.setArgOK(i);
			}
		}
	}
	/** Set parser options -- performs action on the following options:
	  */
	public boolean setOption(String key, Token value) {
		String s = value.getText();
		if (key.equals("buildAST")) {
			if (s.equals("true")) {
				buildAST = true;
			} else if (s.equals("false")) {
				buildAST = false;
			} else {
				tool.error("buildAST option must be true or false", getFilename(), value.getLine());
			}
			return true;
		}
		if (key.equals("interactive")) {
			if (s.equals("true")) {
				interactive = true;
			} else if (s.equals("false")) {
				interactive = false;
			} else {
				tool.error("interactive option must be true or false", getFilename(), value.getLine());
			}
			return true;
		}
		if (key.equals("ASTLabelType")) {
			super.setOption(key, value);
			return true;
		}	
		if (super.setOption(key, value)) {
			return true;
		}
		tool.error("Invalid option: " + key, getFilename(), value.getLine());
		return false;
	}
}