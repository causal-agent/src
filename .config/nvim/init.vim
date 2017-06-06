set hidden
set shortmess=atI
set wildmode=list:longest
set splitbelow splitright

set ignorecase smartcase inccommand=nosplit
set tabstop=4 expandtab shiftwidth=4 shiftround smartindent
set undofile

set title
set scrolloff=1
set number colorcolumn=80,100
set list listchars=tab:»·,trail:·

nmap <leader><leader> :nohlsearch<CR>
command! W w

colorscheme trivial

autocmd BufNewFile,BufRead *.asm,*.mac setfiletype nasm
autocmd FileType sh,zsh,ruby setlocal shiftwidth=2

" Tarmak 1
nnoremap n j
nnoremap j n
nnoremap e k
nnoremap k e
nnoremap N J
nnoremap J N
nnoremap E K
nnoremap K E
vnoremap n j
vnoremap j n
vnoremap e k
vnoremap k e
vnoremap N J
vnoremap J N
vnoremap E K
vnoremap K E

execute pathogen#infect()
