if exists("b:current_syntax")
	finish
endif

runtime! syntax/nroff.vim
unlet! b:current_syntax

setlocal sections+=ShSs
syntax match mdocBlank /^\.$/ conceal
setlocal conceallevel=2

let b:current_syntax = "mdoc"
