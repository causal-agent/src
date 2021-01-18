if !exists('g:rfc_path')
	let g:rfc_path = fnamemodify(exepath('rfc'), ':h:h') . '/share/rfc'
endif

function! s:RFC(number)
	if !empty(a:number)
		let number = str2nr(matchstr(a:number, '\d\+'), 10)
	else
		let number = '-index'
	endif
	let path = expand(g:rfc_path . '/rfc' . number . '.txt')
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
	autocmd BufRead rfc*.txt call s:BufRead()
augroup END
