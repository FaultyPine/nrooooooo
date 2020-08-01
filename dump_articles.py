import ftplib

def dump(fighter):
    ftp.cwd('/fighter/' + fighter + '/model/')
    article_model_list = ftp.nlst()
    ftp.cwd('/fighter/' + fighter + '/motion/')
    article_motion_list = ftp.nlst()

    article_list = list(set().union(article_model_list, article_motion_list))

    vector_name = fighter + "_objs"

    print("    std::vector<std::string> " + vector_name + ";")
    print("    " + vector_name + ".push_back(\"" + fighter + "\");")

    for article in article_list:
        print("    " + vector_name + ".push_back(\"" + fighter + "_" + article + "\");")

    print("    character_objects[\"" + fighter + "\"] = " + vector_name + ";\n")

if __name__ == "__main__":
    ftp = ftplib.FTP("jam1.re")
    ftp.login()

    ftp.cwd('/fighter')

    fighter_list = ftp.nlst()

    for fighter in fighter_list:
        dump(fighter)
