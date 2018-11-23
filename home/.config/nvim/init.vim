set hidden
set undofile
set shortmess=atI
set wildmode=list:longest wildignore=*.o
set splitbelow splitright
command! W w
command! Q q
autocmd BufNewFile,BufRead *.asm,*.mac setfiletype nasm

set tabstop=4 shiftwidth=4 shiftround
set smartindent cinoptions=l1(sU1m1
set ignorecase smartcase inccommand=nosplit
nmap <leader><leader> :nohlsearch<CR>
set foldmethod=syntax foldlevel=99

set title laststatus=1
set scrolloff=1
set noruler colorcolumn=80
set list listchars=tab:\ \ ,trail:·
colorscheme trivial

autocmd TermOpen * setlocal statusline=%{b:term_title}
autocmd BufEnter term://* startinsert
tmap <C-w> <C-\><C-n><C-w>

let g:clipboard = {'copy':{'+':'pbcopy'},'paste':{'+':'pbpaste'}}

nmap <leader>s vip:sort<CR>
nmap <leader>h :$?^#include <<CR>:nohlsearch<CR>vip:sort<CR>
nmap <leader>a :$?^#include <<CR>:nohlsearch<CR>o#include <
nmap <leader>l :0read ~/src/etc/agpl.c<CR>''
