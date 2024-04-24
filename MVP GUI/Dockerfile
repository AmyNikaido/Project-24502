# specifies base image which includes Python 3.10 installed on Alpine Linux
FROM python:3.10-alpine

# sets working directory to /code
WORKDIR /code

# sets environment variable FLASK_APP to app.py
# this is used to specify the entry point to the application
ENV FLASK_APP=app.py

# sets environment variable FLASK_RUN_HOST to 0.0.0.0
# this makes the Flask development server accessible outside the container
ENV FLASK_RUN_HOST=0.0.0.0

# installs necessary dependencies gcc, musl-dec, and linux-headers using apk
RUN apk add --no-cache gcc musl-dev linux-headers

# copies the requirements.txt file to the /code working directory
COPY requirements.txt requirements.txt

# installs dependencies specified in the requirements.txt file using pip
RUN pip install -r requirements.txt

# exposes port 5000 to the container to run the Flask development server
EXPOSE 5000

# copies everything in the current directory to the /code working directory
COPY . .

# runs the command to start the Flask development server and enables debugging
CMD ["flask", "run", "--debug"]