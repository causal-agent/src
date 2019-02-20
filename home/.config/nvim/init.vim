set hidden
set undofile
set shortmess=atI
set wildmode=list:longest wildignore=*.o
set splitbelow splitright
command! W w
command! Q q

set tabstop=4 shiftwidth=4 shiftround
set smartindent cinoptions=l1(sU1m1
set ignorecase smartcase inccommand=nosplit
nmap <leader><leader> :nohlsearch<CR>
set foldmethod=syntax foldlevel=99 foldopen-=block
let asmsyntax = "nasm"
let c_syntax_for_h = 1
let is_posix = 1
let man_hard_wrap = 1

set title laststatus=1
set scrolloff=1
set noruler colorcolumn=80
set list listchars=tab:\ \ ,trail:·
colorscheme trivial

autocmd TermOpen * setlocal statusline=%{b:term_title}
autocmd BufEnter term://* startinsert
tmap <C-w> <C-\><C-n><C-w>

let g:clipboard = {'copy':{'+':'pbcopy'},'paste':{'+':'pbpaste'}}
nmap gp `[v`]

nmap <leader>s vip:sort<CR>
nmap <leader>a :0/^#include <<CR>:nohlsearch<CR>O#include <
nmap <leader>l :0read ~/src/etc/agpl.c<CR>''
