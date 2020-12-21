if !exists('g:rfc_path')
	let g:rfc_path = expand('<sfile>:h:h:h:h') . '/rfc'
endif

function! s:RFC(number)
	let number = (empty(a:number) ? '-index' : str2nr(a:number, 10))
	let path = expand(g:rfc_path . '/rfc' . number . '.txt.gz')
	if filereadable(path)
		execute 'silent' 'noswapfile' 'view' path
	else
		echohl ErrorMsg | echo 'No such RFC' a:number | echohl None
	endif
endfunction

function! s:BufRead()
	setlocal readonly
	setlocal keywordprg=:RFC
	setlocal iskeyword=a-z,A-Z,48-57,.,[,],-,_
	nmap <buffer> <silent> gO :call search('^Table of Contents', 'bcs')<CR>
endfunction

command! -bar -nargs=? RFC call s:RFC(<q-args>)
augroup RFC
	autocmd!
	autocmd BufRead rfc*.txt.gz call s:BufRead()
augroup END
