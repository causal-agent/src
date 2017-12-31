set hidden
set undofile
set shortmess=atI
set wildmode=list:longest
set splitbelow splitright
command! W w
autocmd BufNewFile,BufRead *.asm,*.mac setfiletype nasm

set tabstop=8 expandtab shiftwidth=4 shiftround smartindent
set ignorecase smartcase inccommand=nosplit
nmap <leader><leader> :nohlsearch<CR>
set foldmethod=syntax foldlevel=99

set title
set scrolloff=1
set number colorcolumn=80,100
set list listchars=tab:»·,trail:·
colorscheme trivial

autocmd TermOpen * setlocal nonumber statusline=%{b:term_title}
autocmd BufEnter term://* startinsert
tmap <C-w> <C-\><C-n><C-w>

noremap n j
noremap e k
noremap k n
noremap j e
noremap N J
noremap E K
noremap K N
noremap J E
nmap <C-w>n <C-w>j
nmap <C-w>e <C-w>k
nmap <C-w>N <C-w>J
nmap <C-w>E <C-w>K
