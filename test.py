metting_duration = input("")
battery_lvls = input("")

battery_lvls_list = battery_lvls.split(" ")
for i in battery_lvls_list:
    if i >= metting_duration:
        print(i)