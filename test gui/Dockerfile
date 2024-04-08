FROM python:3.9-slim-buster
WORKDIR /app
COPY ./requirements.txt /app
RUN pip install -r requirements.txt
COPY . .
EXPOSE 4000
ENV FLASK_APP=testGUI.py
CMD ["flask", "run", "--host", "0.0.0.0", "--port=4000"]