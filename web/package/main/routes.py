from flask import render_template
from . import main


@main.route("/")
def info():
    return render_template("index.html")
