package antlr;

import java.util.Hashtable;
import antlr.collections.impl.LList;
import antlr.collections.Stack;
import java.io.IOException;

/** A token stream MUX (multiplexor) knows about n token streams
 *  and can multiplex them onto the same channel for use by token
 *  stream consumer like a parser.  This is a way to have multiple
 *  lexers break up the same input stream for a single parser.
 */
public class TokenStreamSelector implements TokenStream {
	/** The set of inputs to the MUX */
	protected Hashtable inputStreamNames;

	/** The currently-selected token stream input */
	protected TokenStream input;

	/** Used to track stack of input streams */
	protected Stack streamStack = new LList();


public TokenStreamSelector() {
	super();
	inputStreamNames = new Hashtable();
}
	public void addInputStream(TokenStream stream, String key) {
		inputStreamNames.put(key, stream);
	}
	public Token nextToken() throws IOException {
		return input.nextToken();
	}
	public void pop() {
		TokenStream stream = (TokenStream) streamStack.pop();
		select(stream);
	}
	public void push(TokenStream stream) {
		streamStack.push(input); // save current stream
		select(stream);
	}
	public void push(String sname) {
		streamStack.push(input);
		select(sname);
	}
/** Set the stream without pushing old stream */
	public void select(TokenStream stream) {
		input = stream;
	}
	public void select(String sname) throws IllegalArgumentException {
		TokenStream stream = (TokenStream)inputStreamNames.get(sname);
		if ( stream==null ) {
			throw new IllegalArgumentException("TokenStream "+sname+" not found");
		}
		input = stream;
	}
}