So.  We have a very raw lisp interpreter working.  Where to go from here?

Let's make it a little less raw.  How do we do that?

The homoiconicity of lisp means that we can easily write a lisp function that takes as an argument user-written and pre-parsed but otherwise high-level source, and produces as output a lower-level interpreter-friendly form.  In other words, we need a macro system.

What are macros? MORE.

