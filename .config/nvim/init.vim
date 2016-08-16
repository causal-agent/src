set hidden
set shortmess=atI visualbell
set wildmode=list:longest

set ignorecase smartcase
set tabstop=4 expandtab shiftwidth=2 shiftround smartindent
set undofile

set title
set ruler showcmd
set scrolloff=1
set number colorcolumn=80,100
set list listchars=tab:»·,trail:·

nmap <leader><leader> :nohlsearch<CR>

set splitbelow splitright
map <ScrollWheelUp> <C-Y>
map <ScrollWheelDown> <C-E>

colorscheme subtle

autocmd BufNewFile,BufRead *.asm,*.mac setf nasm

execute pathogen#infect()
