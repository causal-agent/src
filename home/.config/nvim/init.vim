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

set title
set scrolloff=1
set number colorcolumn=80,100
set list listchars=tab:\ \ ,trail:·
colorscheme trivial

autocmd TermOpen * setlocal nonumber statusline=%{b:term_title}
autocmd BufEnter term://* startinsert
tmap <C-w> <C-\><C-n><C-w>

let g:clipboard = {'copy':{'+':'pbcopy'},'paste':{'+':'pbpaste'}}

nmap <leader>h :0/^#include </,$?^#include <?sort<CR>:nohlsearch<CR>
nmap <leader>a ?^#include <<CR>:nohlsearch<CR>o#include <
nmap <leader>u :0/^use/,$?^use?sort<CR>:nohlsearch<CR>
nmap <leader>c :0/extern crate/,$?extern crate?sort<CR>:nohlsearch<CR>
