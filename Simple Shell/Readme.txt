Mở Terminal trên hệ điều hành Linux
-->Input dòng lệnh: gcc shell.c -0 shell
-->Input dòng lệnh: ./shell để thực thi chương trình
-->Input commands sau osh>

osh>exit --> chương trình thoát
osh>!! --> chương trình thực thi command gần nhất (nếu có)
Input redirection vd: sort < in.txt --> chương trình đọc input từ file in.txt
Output redirection vd: ls > out.txt --> chương trình in output ra file out.txt
Pipe vd: ls | wc -l --> chương trình lấy output từ command ls làm input của command wc -l 