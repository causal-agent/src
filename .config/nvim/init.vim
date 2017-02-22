set hidden
set shortmess=atI visualbell
set wildmode=list:longest
set splitbelow splitright

set ignorecase smartcase inccommand=split
set tabstop=4 expandtab shiftwidth=4 shiftround smartindent
set undofile

set title
set ruler showcmd
set scrolloff=1
set number colorcolumn=80,100
set list listchars=tab:»·,trail:·

nmap <leader><leader> :nohlsearch<CR>
command! W w

colorscheme lame

autocmd BufNewFile,BufRead *.asm,*.mac setf nasm
autocmd FileType sh,zsh,ruby setlocal shiftwidth=2

execute pathogen#infect()
